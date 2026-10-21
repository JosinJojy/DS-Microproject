/*
 * api.c — HTTP request handlers for MazeLab
 *
 * Routes:
 *   GET  /api/health          — server status
 *   POST /api/solve           — run BFS or DFS
 *   POST /api/maze/generate   — generate a random maze (randomised DFS carving)
 *   GET  (anything else)      — serve static frontend files
 *
 * All handlers validate input thoroughly, return JSON with appropriate
 * Content-Type, and avoid leaking internal details in error responses.
 */

/* Define alloca for MinGW GCC (not defined by default outside MSVC) */
#ifdef __GNUC__
#ifndef alloca
#define alloca(n) __builtin_alloca(n)
#endif
#endif

#define MG_ENABLE_LINES 1
#include "../third_party/mongoose/mongoose.h"
#include "../third_party/cjson/cJSON.h"

#include "api.h"
#include "maze.h"
#include "bfs.h"
#include "dfs.h"
#include "path.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Platform timing                                                      */
/* ------------------------------------------------------------------ */
#ifdef _WIN32
#include <windows.h>
static long long get_time_us(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (long long)(count.QuadPart * 1000000LL / freq.QuadPart);
}
#else
static long long get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
}
#endif

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
#define APP_VERSION "1.0.0"
#define MAX_BODY_LEN (256 * 1024)  /* 256 KB max request body */

static void send_json(struct mg_connection *c, int status,
                      const char *json_str) {
    mg_http_reply(c, status,
                  "Content-Type: application/json\r\n",
                  "%s", json_str);
}

static void send_error(struct mg_connection *c, int status,
                       const char *message) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "error", message);
    char *s = cJSON_PrintUnformatted(obj);
    send_json(c, status, s ? s : "{\"error\":\"internal\"}");
    if (s) free(s);
    cJSON_Delete(obj);
}

/* URI helper: compare mg_str uri against a literal string */
static int uri_eq(struct mg_str uri, const char *literal) {
    struct mg_str lit = mg_str(literal);
    return (uri.len == lit.len &&
            memcmp(uri.buf, lit.buf, uri.len) == 0);
}

/* URI prefix helper: does uri start with prefix? */
static int uri_starts_with(struct mg_str uri, const char *prefix) {
    size_t plen = strlen(prefix);
    return (uri.len >= plen && memcmp(uri.buf, prefix, plen) == 0);
}

/* Method helper */
static int method_eq(struct mg_str method, const char *literal) {
    return mg_strcasecmp(method, mg_str(literal)) == 0;
}

/* ------------------------------------------------------------------ */
/* /api/health                                                          */
/* ------------------------------------------------------------------ */
static void handle_health(struct mg_connection *c) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "status",  "ok");
    cJSON_AddStringToObject(obj, "app",     "MazeLab");
    cJSON_AddStringToObject(obj, "version", APP_VERSION);
    char *s = cJSON_PrintUnformatted(obj);
    send_json(c, 200, s ? s : "{}");
    if (s) free(s);
    cJSON_Delete(obj);
}

/* ------------------------------------------------------------------ */
/* Coord array -> cJSON array                                           */
/* ------------------------------------------------------------------ */
static cJSON *coords_to_json(const Coord *arr, int len) {
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < len; i++) {
        cJSON *cell = cJSON_CreateObject();
        cJSON_AddNumberToObject(cell, "row", arr[i].row);
        cJSON_AddNumberToObject(cell, "col", arr[i].col);
        cJSON_AddItemToArray(a, cell);
    }
    return a;
}

/* ------------------------------------------------------------------ */
/* /api/solve                                                           */
/* ------------------------------------------------------------------ */
static void handle_solve(struct mg_connection *c, struct mg_http_message *hm) {
    /* --- body size guard --- */
    if (hm->body.len > MAX_BODY_LEN) {
        send_error(c, 413, "Request body too large");
        return;
    }

    /* --- parse JSON --- */
    char *body = (char *)malloc(hm->body.len + 1);
    if (!body) { send_error(c, 500, "Out of memory"); return; }
    memcpy(body, hm->body.buf, hm->body.len);
    body[hm->body.len] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) { send_error(c, 400, "Invalid JSON"); return; }

    /* --- extract and validate fields --- */
    cJSON *j_algo  = cJSON_GetObjectItemCaseSensitive(root, "algorithm");
    cJSON *j_rows  = cJSON_GetObjectItemCaseSensitive(root, "rows");
    cJSON *j_cols  = cJSON_GetObjectItemCaseSensitive(root, "cols");
    cJSON *j_grid  = cJSON_GetObjectItemCaseSensitive(root, "grid");
    cJSON *j_start = cJSON_GetObjectItemCaseSensitive(root, "start");
    cJSON *j_end   = cJSON_GetObjectItemCaseSensitive(root, "end");

    if (!cJSON_IsString(j_algo) || !cJSON_IsNumber(j_rows) ||
        !cJSON_IsNumber(j_cols) || !cJSON_IsArray(j_grid)  ||
        !cJSON_IsObject(j_start)|| !cJSON_IsObject(j_end)) {
        cJSON_Delete(root);
        send_error(c, 400, "Missing or invalid required fields");
        return;
    }

    const char *algo = j_algo->valuestring;
    if (strcmp(algo, "bfs") != 0 && strcmp(algo, "dfs") != 0) {
        cJSON_Delete(root);
        send_error(c, 400, "Unknown algorithm; use 'bfs' or 'dfs'");
        return;
    }

    int rows = (int)j_rows->valuedouble;
    int cols = (int)j_cols->valuedouble;
    if (rows < MAZE_MIN_ROWS || rows > MAZE_MAX_ROWS ||
        cols < MAZE_MIN_COLS || cols > MAZE_MAX_COLS) {
        cJSON_Delete(root);
        send_error(c, 400, "Maze dimensions out of allowed range");
        return;
    }

    /* Validate grid array dimensions */
    if (cJSON_GetArraySize(j_grid) != rows) {
        cJSON_Delete(root);
        send_error(c, 400, "Grid row count does not match 'rows'");
        return;
    }

    /* Build flat int array from JSON grid */
    int *flat = (int *)malloc((size_t)rows * (size_t)cols * sizeof(int));
    if (!flat) { cJSON_Delete(root); send_error(c, 500, "Out of memory"); return; }

    for (int r = 0; r < rows; r++) {
        cJSON *row_arr = cJSON_GetArrayItem(j_grid, r);
        if (!cJSON_IsArray(row_arr) || cJSON_GetArraySize(row_arr) != cols) {
            free(flat); cJSON_Delete(root);
            send_error(c, 400, "Grid column count does not match 'cols'");
            return;
        }
        for (int cc = 0; cc < cols; cc++) {
            cJSON *cell = cJSON_GetArrayItem(row_arr, cc);
            if (!cJSON_IsNumber(cell)) {
                free(flat); cJSON_Delete(root);
                send_error(c, 400, "Invalid cell value in grid");
                return;
            }
            int v = (int)cell->valuedouble;
            if (v != 0 && v != 1) {
                free(flat); cJSON_Delete(root);
                send_error(c, 400, "Cell values must be 0 (empty) or 1 (wall)");
                return;
            }
            flat[r * cols + cc] = v;
        }
    }

    /* Extract start/end coords */
    cJSON *js_r = cJSON_GetObjectItemCaseSensitive(j_start, "row");
    cJSON *js_c = cJSON_GetObjectItemCaseSensitive(j_start, "col");
    cJSON *je_r = cJSON_GetObjectItemCaseSensitive(j_end,   "row");
    cJSON *je_c = cJSON_GetObjectItemCaseSensitive(j_end,   "col");

    if (!cJSON_IsNumber(js_r) || !cJSON_IsNumber(js_c) ||
        !cJSON_IsNumber(je_r) || !cJSON_IsNumber(je_c)) {
        free(flat); cJSON_Delete(root);
        send_error(c, 400, "start and end must have numeric row and col");
        return;
    }

    /* Initialise maze */
    Maze m;
    if (maze_init(&m, rows, cols) != 0) {
        free(flat); cJSON_Delete(root);
        send_error(c, 400, "Maze initialisation failed");
        return;
    }
    if (maze_load_grid(&m, flat, rows, cols) != 0) {
        free(flat); cJSON_Delete(root);
        send_error(c, 400, "Invalid grid cell values");
        return;
    }
    free(flat);

    m.start.row = (int)js_r->valuedouble;
    m.start.col = (int)js_c->valuedouble;
    m.end.row   = (int)je_r->valuedouble;
    m.end.col   = (int)je_c->valuedouble;

    cJSON_Delete(root);

    int vr = maze_validate(&m);
    if (vr != 0) {
        const char *msg =
            (vr == -3) ? "Start cell is out of bounds or is a wall" :
            (vr == -4) ? "End cell is out of bounds or is a wall"   :
                         "Maze validation failed";
        send_error(c, 400, msg);
        return;
    }

    /* --- Run algorithm --- */
    long long t_start = get_time_us();
    int    found      = 0;
    int    path_len   = 0;
    int    visited_ct = 0;
    Coord *path_cells = NULL;
    Trace *trace_ptr  = NULL;

    BFSResult bfs_res;
    DFSResult dfs_res;

    if (strcmp(algo, "bfs") == 0) {
        if (bfs_solve(&m, &bfs_res) != 0) {
            send_error(c, 500, "BFS internal error");
            return;
        }
        found      = bfs_res.found;
        path_len   = bfs_res.path_len;
        path_cells = bfs_res.path;
        trace_ptr  = &bfs_res.trace;
        visited_ct = bfs_res.trace.count;
    } else {
        if (dfs_solve(&m, &dfs_res) != 0) {
            send_error(c, 500, "DFS internal error");
            return;
        }
        found      = dfs_res.found;
        path_len   = dfs_res.path_len;
        path_cells = dfs_res.path;
        trace_ptr  = &dfs_res.trace;
        visited_ct = dfs_res.trace.count;
    }

    long long exec_us = get_time_us() - t_start;

    /* --- Build JSON response --- */
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "algorithm",    algo);
    cJSON_AddBoolToObject  (resp, "found",        found);
    cJSON_AddNumberToObject(resp, "visitedCount", visited_ct);
    cJSON_AddNumberToObject(resp, "pathCellCount",
                            found ? path_len : 0);
    cJSON_AddNumberToObject(resp, "pathLength",
                            found ? (path_len > 0 ? path_len - 1 : 0) : 0);
    cJSON_AddNumberToObject(resp, "executionTimeUs", (double)exec_us);

    cJSON_AddItemToObject(resp, "trace",
                          coords_to_json(trace_ptr->cells, trace_ptr->count));
    cJSON_AddItemToObject(resp, "path",
                          coords_to_json(found ? path_cells : NULL,
                                         found ? path_len   : 0));

    char *s = cJSON_PrintUnformatted(resp);
    send_json(c, 200, s ? s : "{}");
    if (s) free(s);
    cJSON_Delete(resp);
}

/* ------------------------------------------------------------------ */
/* /api/maze/generate  — randomised DFS maze carving                   */
/*                                                                      */
/* Algorithm: recursive backtracker (iterative, using explicit stack). */
/* The grid starts all-wall; the algorithm carves passages by moving   */
/* 2 cells in a random direction from the current position, opening   */
/* the intermediate wall cell. This guarantees full connectivity.      */
/*                                                                      */
/* Requires odd dimensions. Even inputs are incremented by 1 then      */
/* capped at MAX.  Seed is optional (defaults to time-based).          */
/* ------------------------------------------------------------------ */
static void carve_passages(uint8_t grid[][MAZE_MAX_COLS],
                            int rows, int cols,
                            int sr, int sc) {
    int stkr[MAZE_MAX_ROWS * MAZE_MAX_COLS];
    int stkc[MAZE_MAX_ROWS * MAZE_MAX_COLS];
    int top = -1;

    grid[sr][sc] = CELL_EMPTY;
    stkr[++top] = sr;
    stkc[top]   = sc;

    int dr[4] = {-2, 0, 2,  0};
    int dc[4] = { 0, 2, 0, -2};

    while (top >= 0) {
        int cr = stkr[top];
        int cc = stkc[top];

        /* Collect valid unvisited neighbours (2 cells away) */
        int ndr[4], ndc[4], ncount = 0;
        for (int d = 0; d < 4; d++) {
            int nr = cr + dr[d];
            int nc = cc + dc[d];
            if (nr > 0 && nr < rows - 1 && nc > 0 && nc < cols - 1 &&
                grid[nr][nc] == CELL_WALL) {
                ndr[ncount] = dr[d];
                ndc[ncount] = dc[d];
                ncount++;
            }
        }

        if (ncount == 0) {
            top--;
            continue;
        }

        int choice = rand() % ncount;
        int nr = cr + ndr[choice];
        int nc = cc + ndc[choice];
        grid[cr + ndr[choice] / 2][cc + ndc[choice] / 2] = CELL_EMPTY;
        grid[nr][nc] = CELL_EMPTY;

        stkr[++top] = nr;
        stkc[top]   = nc;
    }
}

static void handle_generate(struct mg_connection *c,
                             struct mg_http_message *hm) {
    if (hm->body.len > MAX_BODY_LEN) {
        send_error(c, 413, "Request body too large");
        return;
    }

    char *body = (char *)malloc(hm->body.len + 1);
    if (!body) { send_error(c, 500, "Out of memory"); return; }
    memcpy(body, hm->body.buf, hm->body.len);
    body[hm->body.len] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) { send_error(c, 400, "Invalid JSON"); return; }

    cJSON *j_rows = cJSON_GetObjectItemCaseSensitive(root, "rows");
    cJSON *j_cols = cJSON_GetObjectItemCaseSensitive(root, "cols");
    cJSON *j_seed = cJSON_GetObjectItemCaseSensitive(root, "seed");

    if (!cJSON_IsNumber(j_rows) || !cJSON_IsNumber(j_cols)) {
        cJSON_Delete(root);
        send_error(c, 400, "rows and cols must be numbers");
        return;
    }

    int rows = (int)j_rows->valuedouble;
    int cols = (int)j_cols->valuedouble;

    /* Enforce odd dimensions */
    if (rows % 2 == 0) rows++;
    if (cols % 2 == 0) cols++;
    if (rows < MAZE_MIN_ROWS) rows = (MAZE_MIN_ROWS % 2 == 0) ? MAZE_MIN_ROWS + 1 : MAZE_MIN_ROWS;
    if (cols < MAZE_MIN_COLS) cols = (MAZE_MIN_COLS % 2 == 0) ? MAZE_MIN_COLS + 1 : MAZE_MIN_COLS;
    if (rows > MAZE_MAX_ROWS) rows = (MAZE_MAX_ROWS % 2 == 0) ? MAZE_MAX_ROWS - 1 : MAZE_MAX_ROWS;
    if (cols > MAZE_MAX_COLS) cols = (MAZE_MAX_COLS % 2 == 0) ? MAZE_MAX_COLS - 1 : MAZE_MAX_COLS;

    unsigned int seed;
    if (cJSON_IsNumber(j_seed)) {
        seed = (unsigned int)(long long)j_seed->valuedouble;
    } else {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);

    cJSON_Delete(root);

    Maze m;
    m.rows = rows;
    m.cols = cols;
    for (int r = 0; r < rows; r++)
        for (int c2 = 0; c2 < cols; c2++)
            m.grid[r][c2] = CELL_WALL;

    carve_passages(m.grid, rows, cols, 1, 1);

    m.start.row = 1;
    m.start.col = 1;
    m.end.row   = rows - 2;
    m.end.col   = cols - 2;
    m.grid[m.start.row][m.start.col] = CELL_EMPTY;
    m.grid[m.end.row][m.end.col]     = CELL_EMPTY;

    cJSON *resp   = cJSON_CreateObject();
    cJSON *j_grid = cJSON_CreateArray();
    for (int r = 0; r < rows; r++) {
        cJSON *row_arr = cJSON_CreateArray();
        for (int c2 = 0; c2 < cols; c2++) {
            cJSON_AddItemToArray(row_arr, cJSON_CreateNumber(m.grid[r][c2]));
        }
        cJSON_AddItemToArray(j_grid, row_arr);
    }

    cJSON_AddNumberToObject(resp, "rows", rows);
    cJSON_AddNumberToObject(resp, "cols", cols);
    cJSON_AddItemToObject  (resp, "grid", j_grid);

    cJSON *j_st = cJSON_CreateObject();
    cJSON_AddNumberToObject(j_st, "row", m.start.row);
    cJSON_AddNumberToObject(j_st, "col", m.start.col);
    cJSON_AddItemToObject(resp, "start", j_st);

    cJSON *j_en = cJSON_CreateObject();
    cJSON_AddNumberToObject(j_en, "row", m.end.row);
    cJSON_AddNumberToObject(j_en, "col", m.end.col);
    cJSON_AddItemToObject(resp, "end", j_en);

    cJSON_AddNumberToObject(resp, "seed", (double)seed);

    char *s = cJSON_PrintUnformatted(resp);
    send_json(c, 200, s ? s : "{}");
    if (s) free(s);
    cJSON_Delete(resp);
}

/* ------------------------------------------------------------------ */
/* Dispatcher                                                           */
/* ------------------------------------------------------------------ */
void api_dispatch(struct mg_connection *c, struct mg_http_message *hm,
                  const char *frontend_root) {
    struct mg_str uri    = hm->uri;
    struct mg_str method = hm->method;

    /* Health */
    if (uri_eq(uri, "/api/health")) {
        if (method_eq(method, "GET")) {
            handle_health(c);
        } else {
            send_error(c, 405, "Method not allowed");
        }
        return;
    }

    /* Solve */
    if (uri_eq(uri, "/api/solve")) {
        if (method_eq(method, "POST")) {
            handle_solve(c, hm);
        } else {
            send_error(c, 405, "Method not allowed");
        }
        return;
    }

    /* Generate */
    if (uri_eq(uri, "/api/maze/generate")) {
        if (method_eq(method, "POST")) {
            handle_generate(c, hm);
        } else {
            send_error(c, 405, "Method not allowed");
        }
        return;
    }

    /* Unknown /api/ route */
    if (uri_starts_with(uri, "/api/")) {
        send_error(c, 404, "API route not found");
        return;
    }

    /* Static file serving — frontend assets */
    struct mg_http_serve_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.root_dir = frontend_root;
    mg_http_serve_dir(c, hm, &opts);
}
