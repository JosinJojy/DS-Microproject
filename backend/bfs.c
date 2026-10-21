#include "bfs.h"
#include "queue.h"
#include <string.h>

/*
 * Neighbor direction order (deterministic, applied in this exact sequence):
 *   0: Up    (-1,  0)
 *   1: Right  (0, +1)
 *   2: Down  (+1,  0)
 *   3: Left   (0, -1)
 */
static const int DR[4] = {-1, 0, 1,  0};
static const int DC[4] = { 0, 1, 0, -1};

int bfs_solve(const Maze *m, BFSResult *result) {
    /* ---- initialise result ---- */
    result->found    = 0;
    result->path_len = 0;
    trace_init(&result->trace);

    ParentGrid pg;
    parent_init(&pg);

    /* visited array: 1 when a cell has been enqueued */
    uint8_t visited[MAZE_MAX_ROWS][MAZE_MAX_COLS];
    memset(visited, 0, sizeof(visited));

    Queue q;
    queue_init(&q);

    Coord start = m->start;
    Coord end   = m->end;

    /* Enqueue start, mark discovered */
    if (queue_enqueue(&q, start) != 0) return -1;
    visited[start.row][start.col] = 1;
    trace_add(&result->trace, start);

    /* Special case: start == end */
    if (coord_eq(start, end)) {
        result->found    = 1;
        result->path[0]  = start;
        result->path_len = 1;
        return 0;
    }

    int reached = 0;

    while (!queue_is_empty(&q)) {
        Coord cur;
        if (queue_dequeue(&q, &cur) != 0) return -1;

        for (int d = 0; d < 4; d++) {
            Coord nb;
            nb.row = cur.row + DR[d];
            nb.col = cur.col + DC[d];

            if (!maze_in_bounds(m, nb.row, nb.col)) continue;
            if (m->grid[nb.row][nb.col] == CELL_WALL)  continue;
            if (visited[nb.row][nb.col])                continue;

            visited[nb.row][nb.col] = 1;
            pg.cell[nb.row][nb.col] = cur;   /* record parent */
            trace_add(&result->trace, nb);

            if (coord_eq(nb, end)) {
                reached = 1;
                break;
            }
            if (queue_enqueue(&q, nb) != 0) return -1;
        }
        if (reached) break;
    }

    if (reached) {
        result->found    = 1;
        result->path_len = path_reconstruct(&pg, start, end,
                                            result->path, MAX_PATH_LEN);
    }
    return 0;
}
