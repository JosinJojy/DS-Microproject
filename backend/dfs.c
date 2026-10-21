#include "dfs.h"
#include "stack.h"
#include <string.h>

/*
 * Neighbors are pushed onto the stack in REVERSE of the desired processing
 * order.  Because a stack reverses insertion order, pushing Left → Down →
 * Right → Up means Up is processed (popped) first — matching BFS direction
 * order for determinism.
 *
 * Push order: Left (0,-1), Down (+1,0), Right (0,+1), Up (-1,0)
 */
static const int DR[4] = { 0, 1,  0, -1};
static const int DC[4] = {-1, 0,  1,  0};

int dfs_solve(const Maze *m, DFSResult *result) {
    result->found    = 0;
    result->path_len = 0;
    trace_init(&result->trace);

    ParentGrid pg;
    parent_init(&pg);

    uint8_t visited[MAZE_MAX_ROWS][MAZE_MAX_COLS];
    memset(visited, 0, sizeof(visited));

    Stack s;
    stack_init(&s);

    Coord start = m->start;
    Coord end   = m->end;

    /* Special case: start == end */
    if (coord_eq(start, end)) {
        result->found    = 1;
        result->path[0]  = start;
        result->path_len = 1;
        trace_add(&result->trace, start);
        return 0;
    }

    if (stack_push(&s, start) != 0) return -1;

    while (!stack_is_empty(&s)) {
        Coord cur;
        if (stack_pop(&s, &cur) != 0) return -1;

        /* Skip if already visited (a cell may be pushed multiple times
         * before it is popped — we only process it on the first pop). */
        if (visited[cur.row][cur.col]) continue;

        visited[cur.row][cur.col] = 1;
        trace_add(&result->trace, cur);

        if (coord_eq(cur, end)) {
            result->found    = 1;
            result->path_len = path_reconstruct(&pg, start, end,
                                                result->path, MAX_PATH_LEN);
            return 0;
        }

        /* Push neighbors in push-order (Left, Down, Right, Up).
         * The stack inverts this so Up is explored first. */
        for (int d = 0; d < 4; d++) {
            Coord nb;
            nb.row = cur.row + DR[d];
            nb.col = cur.col + DC[d];

            if (!maze_in_bounds(m, nb.row, nb.col)) continue;
            if (m->grid[nb.row][nb.col] == CELL_WALL)  continue;
            if (visited[nb.row][nb.col])                continue;

            /* Record parent only if nb has not been given a parent yet.
             * This preserves the first-time-discovered parent. */
            if (coord_is_none(pg.cell[nb.row][nb.col])) {
                pg.cell[nb.row][nb.col] = cur;
            }
            if (stack_push(&s, nb) != 0) return -1;
        }
    }

    /* Stack exhausted without reaching the end — no path */
    return 0;
}
