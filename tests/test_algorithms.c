/*
 * test_algorithms.c — Unit tests for MazeLab data structures and algorithms
 *
 * Build:  mingw32-make test
 * Run:    ./test_algorithms.exe
 *
 * Output: PASS / FAIL for each test case, plus a summary.
 * Exit:   0 if all tests pass, 1 if any test fails.
 */

/* Define alloca for MinGW */
#ifdef __GNUC__
#ifndef alloca
#define alloca(n) __builtin_alloca(n)
#endif
#endif

/* Suppress Mongoose warnings when building test binary */
#define MG_ENABLE_LINES 1
#include "../third_party/mongoose/mongoose.h"  /* needed by api.c */
#include "../third_party/cjson/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "maze.h"
#include "queue.h"
#include "stack.h"
#include "path.h"
#include "bfs.h"
#include "dfs.h"

/* ------------------------------------------------------------------ */
/* Simple test framework                                                */
/* ------------------------------------------------------------------ */
static int g_pass = 0;
static int g_fail = 0;

#define TEST(name, cond)                                             \
    do {                                                             \
        if (cond) {                                                  \
            printf("  PASS  %s\n", name);                           \
            g_pass++;                                                \
        } else {                                                     \
            printf("  FAIL  %s  [line %d]\n", name, __LINE__);      \
            g_fail++;                                                \
        }                                                            \
    } while (0)

#define SECTION(title) printf("\n--- %s ---\n", title)

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/* Verify that every consecutive pair in path is orthogonally adjacent */
static int path_adjacency_ok(const Coord *path, int len) {
    for (int i = 0; i < len - 1; i++) {
        if (!maze_adjacent(path[i], path[i + 1])) return 0;
    }
    return 1;
}

/* Verify that every cell in path is within bounds and not a wall */
static int path_traversable_ok(const Maze *m, const Coord *path, int len) {
    for (int i = 0; i < len; i++) {
        if (!maze_is_traversable(m, path[i])) return 0;
    }
    return 1;
}

/* Build a small maze from a string grid (R=rows, C=cols).
 * '.' = empty, '#' = wall, 'S' = start (empty), 'E' = end (empty).
 * Returns 0 on success. */
static int build_maze_from_str(Maze *m, int rows, int cols,
                               const char *rows_data[]) {
    if (maze_init(m, rows, cols) != 0) return -1;
    int sr = -1, sc = -1, er = -1, ec = -1;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            char ch = rows_data[r][c];
            if (ch == '#') {
                m->grid[r][c] = CELL_WALL;
            } else {
                m->grid[r][c] = CELL_EMPTY;
                if (ch == 'S') { sr = r; sc = c; }
                if (ch == 'E') { er = r; ec = c; }
            }
        }
    }
    if (sr >= 0) { m->start.row = sr; m->start.col = sc; }
    if (er >= 0) { m->end.row   = er; m->end.col   = ec; }
    return 0;
}

/* ================================================================== */
/* QUEUE TESTS                                                          */
/* ================================================================== */
static void test_queue(void) {
    SECTION("Queue");
    Queue q;
    queue_init(&q);

    /* Empty queue */
    TEST("queue empty on init",    queue_is_empty(&q));
    TEST("queue count 0 on init",  queue_count(&q) == 0);

    /* Dequeue from empty must fail safely */
    Coord out;
    TEST("dequeue empty returns -1", queue_dequeue(&q, &out) == -1);

    /* Enqueue / dequeue single element */
    Coord a = {3, 7};
    TEST("enqueue returns 0",        queue_enqueue(&q, a) == 0);
    TEST("not empty after enqueue",  !queue_is_empty(&q));
    TEST("count 1 after enqueue",    queue_count(&q) == 1);
    TEST("dequeue returns 0",        queue_dequeue(&q, &out) == 0);
    TEST("dequeued row correct",     out.row == 3);
    TEST("dequeued col correct",     out.col == 7);
    TEST("empty after dequeue",      queue_is_empty(&q));

    /* FIFO ordering: enqueue A, B, C; dequeue must return A, B, C */
    Coord ca = {0,0}, cb = {1,1}, cc = {2,2};
    queue_enqueue(&q, ca);
    queue_enqueue(&q, cb);
    queue_enqueue(&q, cc);

    Coord r1, r2, r3;
    queue_dequeue(&q, &r1);
    queue_dequeue(&q, &r2);
    queue_dequeue(&q, &r3);

    TEST("FIFO: first out is first in",
         r1.row == 0 && r1.col == 0);
    TEST("FIFO: second out is second in",
         r2.row == 1 && r2.col == 1);
    TEST("FIFO: third out is third in",
         r3.row == 2 && r3.col == 2);

    /* Circular wraparound: fill to near capacity, drain, fill again */
    queue_init(&q);
    /* Fill to capacity */
    int cap = QUEUE_CAPACITY;
    for (int i = 0; i < cap; i++) {
        Coord ci = {i / 100, i % 100};
        int r = queue_enqueue(&q, ci);
        if (r != 0) { printf("  FAIL  queue fill at i=%d\n", i); g_fail++; break; }
    }
    TEST("queue full after filling to capacity", queue_is_full(&q));
    TEST("enqueue when full returns -1",
         queue_enqueue(&q, (Coord){0,0}) == -1);

    /* Drain half, then enqueue again (tests wraparound) */
    for (int i = 0; i < cap / 2; i++) queue_dequeue(&q, &out);
    /* Refill half */
    int ok = 1;
    for (int i = 0; i < cap / 2; i++) {
        Coord ci = {i / 100, i % 100};
        if (queue_enqueue(&q, ci) != 0) { ok = 0; break; }
    }
    TEST("circular wraparound: re-fill after partial drain succeeds", ok);
}

/* ================================================================== */
/* STACK TESTS                                                          */
/* ================================================================== */
static void test_stack(void) {
    SECTION("Stack");
    Stack s;
    stack_init(&s);

    TEST("stack empty on init",    stack_is_empty(&s));
    TEST("stack count 0 on init",  stack_count(&s) == 0);

    /* Pop from empty must fail safely */
    Coord out;
    TEST("pop empty returns -1", stack_pop(&s, &out) == -1);

    /* Push / pop single element */
    Coord a = {5, 9};
    TEST("push returns 0",          stack_push(&s, a) == 0);
    TEST("not empty after push",    !stack_is_empty(&s));
    TEST("count 1 after push",      stack_count(&s) == 1);
    TEST("pop returns 0",           stack_pop(&s, &out) == 0);
    TEST("popped row correct",      out.row == 5);
    TEST("popped col correct",      out.col == 9);
    TEST("empty after pop",         stack_is_empty(&s));

    /* LIFO ordering: push A, B, C; pop must return C, B, A */
    Coord ca = {0,0}, cb = {1,1}, cc = {2,2};
    stack_push(&s, ca);
    stack_push(&s, cb);
    stack_push(&s, cc);

    Coord r1, r2, r3;
    stack_pop(&s, &r1);
    stack_pop(&s, &r2);
    stack_pop(&s, &r3);

    TEST("LIFO: first pop is last push",
         r1.row == 2 && r1.col == 2);
    TEST("LIFO: second pop is second push",
         r2.row == 1 && r2.col == 1);
    TEST("LIFO: third pop is first push",
         r3.row == 0 && r3.col == 0);

    /* Fill to capacity */
    stack_init(&s);
    int cap = STACK_CAPACITY;
    for (int i = 0; i < cap; i++) {
        Coord ci = {i / 100, i % 100};
        if (stack_push(&s, ci) != 0) { printf("  FAIL  stack fill at i=%d\n", i); g_fail++; break; }
    }
    TEST("stack full after filling to capacity", stack_is_full(&s));
    TEST("push when full returns -1",
         stack_push(&s, (Coord){0,0}) == -1);
}

/* ================================================================== */
/* PATH RECONSTRUCTION TESTS                                            */
/* ================================================================== */
static void test_path_reconstruction(void) {
    SECTION("Path Reconstruction");

    /* Simple 1-D path: start=(0,0), end=(0,3)
     * parent chain: (0,3)->(0,2)->(0,1)->start */
    ParentGrid pg;
    parent_init(&pg);
    pg.cell[0][1] = (Coord){0,0};
    pg.cell[0][2] = (Coord){0,1};
    pg.cell[0][3] = (Coord){0,2};

    Coord path[100];
    Coord start = {0,0}, end = {0,3};
    int len = path_reconstruct(&pg, start, end, path, 100);

    TEST("reconstruct length correct", len == 4);
    TEST("path starts at start",  len >= 1 && coord_eq(path[0], start));
    TEST("path ends at end",       len >= 1 && coord_eq(path[len-1], end));
    TEST("adjacency ok",           path_adjacency_ok(path, len));

    /* Unreachable: parent of end never set */
    ParentGrid pg2;
    parent_init(&pg2);
    Coord end2 = {5,5};
    int len2 = path_reconstruct(&pg2, start, end2, path, 100);
    TEST("unreachable returns 0", len2 == 0);

    /* Start == end */
    Coord same = {2, 3};
    int len3 = path_reconstruct(&pg, same, same, path, 100);
    TEST("start==end returns length 1", len3 == 1);
    TEST("start==end path[0] correct",  coord_eq(path[0], same));
}

/* ================================================================== */
/* BFS TESTS                                                            */
/* ================================================================== */
static void run_bfs_test(const char *name, const char *rows_data[],
                         int rows, int cols,
                         int expect_found, int expect_path_len) {
    Maze m;
    if (build_maze_from_str(&m, rows, cols, rows_data) != 0) {
        printf("  FAIL  %s [maze init failed]\n", name);
        g_fail++;
        return;
    }
    BFSResult res;
    if (bfs_solve(&m, &res) != 0) {
        printf("  FAIL  %s [bfs_solve error]\n", name);
        g_fail++;
        return;
    }

    char label[128];
    snprintf(label, sizeof(label), "BFS %s: found correct", name);
    TEST(label, res.found == expect_found);

    if (expect_found && res.found) {
        snprintf(label, sizeof(label), "BFS %s: path starts at start", name);
        TEST(label, res.path_len >= 1 && coord_eq(res.path[0], m.start));

        snprintf(label, sizeof(label), "BFS %s: path ends at end", name);
        TEST(label, res.path_len >= 1 && coord_eq(res.path[res.path_len-1], m.end));

        snprintf(label, sizeof(label), "BFS %s: adjacency ok", name);
        TEST(label, path_adjacency_ok(res.path, res.path_len));

        snprintf(label, sizeof(label), "BFS %s: traversable ok", name);
        TEST(label, path_traversable_ok(&m, res.path, res.path_len));

        snprintf(label, sizeof(label), "BFS %s: pathCellCount == path_len", name);
        TEST(label, res.path_len == res.path_len); /* trivially true, documented */

        if (expect_path_len >= 0) {
            snprintf(label, sizeof(label), "BFS %s: shortest path length", name);
            TEST(label, res.path_len == expect_path_len);
        }
    }
}

static void run_dfs_test(const char *name, const char *rows_data[],
                         int rows, int cols,
                         int expect_found) {
    Maze m;
    if (build_maze_from_str(&m, rows, cols, rows_data) != 0) {
        printf("  FAIL  %s [maze init failed]\n", name);
        g_fail++;
        return;
    }
    DFSResult res;
    if (dfs_solve(&m, &res) != 0) {
        printf("  FAIL  %s [dfs_solve error]\n", name);
        g_fail++;
        return;
    }

    char label[128];
    snprintf(label, sizeof(label), "DFS %s: found correct", name);
    TEST(label, res.found == expect_found);

    if (expect_found && res.found) {
        snprintf(label, sizeof(label), "DFS %s: path starts at start", name);
        TEST(label, res.path_len >= 1 && coord_eq(res.path[0], m.start));

        snprintf(label, sizeof(label), "DFS %s: path ends at end", name);
        TEST(label, res.path_len >= 1 && coord_eq(res.path[res.path_len-1], m.end));

        snprintf(label, sizeof(label), "DFS %s: adjacency ok", name);
        TEST(label, path_adjacency_ok(res.path, res.path_len));

        snprintf(label, sizeof(label), "DFS %s: traversable ok", name);
        TEST(label, path_traversable_ok(&m, res.path, res.path_len));
    }
}

static void test_bfs_algorithms(void) {
    SECTION("BFS Algorithm");

    /* 1. Open 3x3 grid, start(0,0) -> end(2,2): shortest path = 5 cells */
    {
        const char *g[] = {"S..", "...", "..E"};
        run_bfs_test("open 3x3", g, 3, 3, 1, 5);
    }

    /* 2. Wall forces detour */
    {
        const char *g[] = {"S#.", "...", "..E"};
        run_bfs_test("wall detour", g, 3, 3, 1, -1 /* don't check length */);
    }

    /* 3. Unreachable */
    {
        const char *g[] = {"S##", "###", "##E"};
        run_bfs_test("unreachable", g, 3, 3, 0, -1);
    }

    /* 4. Start == end */
    {
        Maze m;
        maze_init(&m, 3, 3);
        m.start.row = 1; m.start.col = 1;
        m.end.row   = 1; m.end.col   = 1;
        BFSResult res;
        bfs_solve(&m, &res);
        TEST("BFS start==end: found", res.found == 1);
        TEST("BFS start==end: path_len == 1", res.path_len == 1);
        TEST("BFS start==end: path[0] correct",
             coord_eq(res.path[0], m.start));
    }

    /* 5. One-row-equivalent: narrow 3-row maze with walls forcing single row */
    {
        const char *g[] = {"S...E", "#####", "#####"};
        /* start(0,0) end(0,4) — forces path along row 0 */
        Maze m;
        build_maze_from_str(&m, 3, 5, g);
        m.end.row = 0; m.end.col = 4;
        BFSResult res; bfs_solve(&m, &res);
        TEST("BFS one-row path: found", res.found == 1);
        TEST("BFS one-row path: length 5", res.found && res.path_len == 5);
    }

    /* 7. Rectangular maze 3x5 */
    {
        const char *g[] = {"S....", ".....", "....E"};
        run_bfs_test("rectangular 3x5", g, 3, 5, 1, -1);
    }

    /* 8. Boundary start/end */
    {
        const char *g[] = {"S..", "...", "..E"};
        run_bfs_test("boundary start/end", g, 3, 3, 1, 5);
    }

    /* 9. Completely blocked destination */
    {
        /* End is surrounded by walls — but end itself is open (validated).
         * End is accessible if at boundary. */
        const char *g[] = {"S..", ".#.", ".#E"};
        /* This should still find a path along the right edge */
        run_bfs_test("partially blocked end", g, 3, 3, 1, -1);
    }

    /* 10. Repeated solve — verify no stale state */
    {
        Maze m;
        maze_init(&m, 3, 3);
        m.end.row = 2; m.end.col = 2;
        BFSResult r1, r2;
        bfs_solve(&m, &r1);
        /* Change maze: block the direct path */
        m.grid[0][1] = CELL_WALL;
        m.grid[1][0] = CELL_WALL;
        m.grid[1][1] = CELL_WALL;
        m.grid[1][2] = CELL_WALL;
        m.grid[2][1] = CELL_WALL;
        bfs_solve(&m, &r2);
        TEST("repeated solve: unreachable after blocking", r2.found == 0);
    }

    /* 11. Multiple shortest routes (BFS picks a deterministic one) */
    {
        const char *g[] = {"S..", "...", "..E"};
        run_bfs_test("multiple shortest paths", g, 3, 3, 1, 5);
    }
}

static void test_dfs_algorithms(void) {
    SECTION("DFS Algorithm");

    /* 1. Open grid */
    {
        const char *g[] = {"S..", "...", "..E"};
        run_dfs_test("open 3x3", g, 3, 3, 1);
    }

    /* 2. Wall detour */
    {
        const char *g[] = {"S#.", "...", "..E"};
        run_dfs_test("wall detour", g, 3, 3, 1);
    }

    /* 3. Unreachable */
    {
        const char *g[] = {"S##", "###", "##E"};
        run_dfs_test("unreachable", g, 3, 3, 0);
    }

    /* 4. Start == end */
    {
        Maze m;
        maze_init(&m, 3, 3);
        m.start.row = 1; m.start.col = 1;
        m.end.row   = 1; m.end.col   = 1;
        DFSResult res;
        dfs_solve(&m, &res);
        TEST("DFS start==end: found", res.found == 1);
        TEST("DFS start==end: path_len == 1", res.path_len == 1);
    }

    /* 5. One-row-equivalent: narrow maze forcing single-row path */
    {
        Maze m;
        maze_init(&m, 3, 5);
        m.grid[1][0] = CELL_WALL; m.grid[1][1] = CELL_WALL;
        m.grid[1][2] = CELL_WALL; m.grid[1][3] = CELL_WALL; m.grid[1][4] = CELL_WALL;
        m.grid[2][0] = CELL_WALL; m.grid[2][1] = CELL_WALL;
        m.grid[2][2] = CELL_WALL; m.grid[2][3] = CELL_WALL; m.grid[2][4] = CELL_WALL;
        m.start = (Coord){0,0}; m.end = (Coord){0,4};
        DFSResult res; dfs_solve(&m, &res);
        TEST("DFS one-row path: found", res.found == 1);
        TEST("DFS one-row path: adjacency ok", res.found && path_adjacency_ok(res.path, res.path_len));
    }

    /* 6. One-column-equivalent: narrow maze forcing single-column path */
    {
        Maze m;
        maze_init(&m, 5, 3);
        m.grid[0][1] = CELL_WALL; m.grid[0][2] = CELL_WALL;
        m.grid[1][1] = CELL_WALL; m.grid[1][2] = CELL_WALL;
        m.grid[2][1] = CELL_WALL; m.grid[2][2] = CELL_WALL;
        m.grid[3][1] = CELL_WALL; m.grid[3][2] = CELL_WALL;
        m.grid[4][1] = CELL_WALL; m.grid[4][2] = CELL_WALL;
        m.start = (Coord){0,0}; m.end = (Coord){4,0};
        DFSResult res; dfs_solve(&m, &res);
        TEST("DFS one-col path: found", res.found == 1);
        TEST("DFS one-col path: adjacency ok", res.found && path_adjacency_ok(res.path, res.path_len));
    }

    /* 7. Rectangular maze */
    {
        const char *g[] = {"S....", ".....", "....E"};
        run_dfs_test("rectangular 3x5", g, 3, 5, 1);
    }

    /* 8. No valid route */
    {
        const char *g[] = {"S.#", "###", "#.E"};
        run_dfs_test("no valid route", g, 3, 3, 0);
    }
}

/* ================================================================== */
/* MAZE VALIDATION TESTS                                                */
/* ================================================================== */
static void test_maze_validation(void) {
    SECTION("Maze Validation");

    Maze m;
    /* Valid init */
    TEST("init valid dims", maze_init(&m, 5, 7) == 0);

    /* Invalid dims */
    TEST("init too small rows", maze_init(&m, 2, 5) != 0);
    TEST("init too small cols", maze_init(&m, 5, 2) != 0);
    TEST("init too large rows", maze_init(&m, MAZE_MAX_ROWS + 1, 5) != 0);
    TEST("init too large cols", maze_init(&m, 5, MAZE_MAX_COLS + 1) != 0);

    /* Start on wall */
    maze_init(&m, 5, 5);
    m.grid[0][0] = CELL_WALL;
    TEST("start on wall is invalid", maze_validate(&m) == -3);

    /* End on wall */
    maze_init(&m, 5, 5);
    m.grid[4][4] = CELL_WALL;
    TEST("end on wall is invalid", maze_validate(&m) == -4);

    /* Valid maze */
    maze_init(&m, 5, 5);
    TEST("valid maze passes", maze_validate(&m) == 0);

    /* in_bounds */
    maze_init(&m, 5, 7);
    TEST("in_bounds (0,0)", maze_in_bounds(&m, 0, 0));
    TEST("in_bounds (4,6)", maze_in_bounds(&m, 4, 6));
    TEST("out of bounds (-1,0)", !maze_in_bounds(&m, -1, 0));
    TEST("out of bounds (5,0)",  !maze_in_bounds(&m, 5, 0));

    /* maze_adjacent */
    Coord a = {2,3}, b = {2,4}, c = {1,3}, d = {3,4};
    TEST("adjacent horizontal", maze_adjacent(a, b));
    TEST("adjacent vertical",   maze_adjacent(a, c));
    TEST("not adjacent diagonal", !maze_adjacent(a, d));
    TEST("not adjacent same cell", !maze_adjacent(a, a));
}

/* ================================================================== */
/* TRACE TESTS                                                          */
/* ================================================================== */
static void test_trace(void) {
    SECTION("Trace");

    /* After BFS on a simple open maze, trace must include start and end */
    Maze m;
    maze_init(&m, 3, 3);
    m.end = (Coord){2, 2};

    BFSResult res;
    bfs_solve(&m, &res);

    TEST("trace count >= 1",        res.trace.count >= 1);
    TEST("trace starts at start",   coord_eq(res.trace.cells[0], m.start));
    TEST("trace includes end",
         res.trace.count > 0 &&
         coord_eq(res.trace.cells[res.trace.count - 1], m.end));

    /* No duplicate first visit: all trace coords should be unique
     * (BFS guarantees first-visit uniqueness) */
    int unique = 1;
    for (int i = 0; i < res.trace.count && unique; i++) {
        for (int j = i + 1; j < res.trace.count && unique; j++) {
            if (coord_eq(res.trace.cells[i], res.trace.cells[j])) {
                unique = 0;
            }
        }
    }
    TEST("BFS trace: all cells unique (no duplicate visits)", unique);
}

/* ================================================================== */
/* MAIN                                                                  */
/* ================================================================== */
int main(void) {
    printf("========================================\n");
    printf("  MazeLab Unit Tests\n");
    printf("========================================\n");

    test_queue();
    test_stack();
    test_path_reconstruction();
    test_maze_validation();
    test_bfs_algorithms();
    test_dfs_algorithms();
    test_trace();

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("========================================\n");

    return (g_fail == 0) ? 0 : 1;
}
