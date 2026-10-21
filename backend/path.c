#include "path.h"
#include <string.h>

void parent_init(ParentGrid *pg) {
    Coord none = coord_none();
    for (int r = 0; r < MAZE_MAX_ROWS; r++) {
        for (int c = 0; c < MAZE_MAX_COLS; c++) {
            pg->cell[r][c] = none;
        }
    }
}

int path_reconstruct(const ParentGrid *pg,
                     Coord start, Coord end,
                     Coord path_out[], int max_len) {
    if (max_len <= 0) return 0;

    /* Special case: start == end */
    if (coord_eq(start, end)) {
        path_out[0] = start;
        return 1;
    }

    /* Check that end was reached (its parent must be set) */
    if (coord_is_none(pg->cell[end.row][end.col])) {
        return 0; /* no path */
    }

    /* Walk back from end to start using parent links.
     * Temporarily store the reversed path in path_out. */
    Coord tmp[MAX_PATH_LEN];
    int   len = 0;

    Coord cur = end;
    while (!coord_eq(cur, start)) {
        if (len >= max_len) return 0; /* safety: path longer than buffer */
        tmp[len++] = cur;

        Coord par = pg->cell[cur.row][cur.col];
        if (coord_is_none(par)) {
            /* Reached an unvisited cell before hitting start — broken chain */
            return 0;
        }
        cur = par;
    }
    /* Append start */
    if (len >= max_len) return 0;
    tmp[len++] = start;

    /* Reverse into path_out so the result is start → end */
    for (int i = 0; i < len; i++) {
        path_out[i] = tmp[len - 1 - i];
    }
    return len;
}

/* ------------------------------------------------------------------ */
/* Trace                                                                */
/* ------------------------------------------------------------------ */

void trace_init(Trace *t) {
    t->count = 0;
}

int trace_add(Trace *t, Coord c) {
    if (t->count >= MAX_PATH_LEN) return -1;
    t->cells[t->count++] = c;
    return 0;
}
