#ifndef PATH_H
#define PATH_H

#include "maze.h"

/* ------------------------------------------------------------------ */
/* Parent-tracking array and path reconstruction                        */
/*                                                                     */
/* The parent array stores, for each cell (r, c), which cell           */
/* discovered it.  A sentinel value of (-1, -1) means "no parent"      */
/* (used for the start cell and for unvisited cells).                   */
/* ------------------------------------------------------------------ */

/* Maximum path length is bounded by the total number of cells. */
#define MAX_PATH_LEN  (MAZE_MAX_ROWS * MAZE_MAX_COLS)

/* Parent grid: for each cell (r,c) stores the Coord that first reached it. */
typedef struct {
    Coord cell[MAZE_MAX_ROWS][MAZE_MAX_COLS];
} ParentGrid;

/** Sentinel value meaning "no parent / not visited". */
static inline Coord coord_none(void) {
    Coord c; c.row = -1; c.col = -1; return c;
}

static inline int coord_is_none(Coord c) {
    return (c.row == -1 && c.col == -1);
}

static inline int coord_eq(Coord a, Coord b) {
    return (a.row == b.row && a.col == b.col);
}

/**
 * Initialise all entries in the parent grid to the sentinel value.
 */
void parent_init(ParentGrid *pg);

/**
 * Reconstruct the path from start to end using the parent grid.
 *
 * Writes at most max_len coordinates into path_out[], starting with the
 * start cell and ending with the end cell.
 *
 * Returns the number of cells written (>= 1 if start == end),
 * or 0 if no path exists (end has no parent chain back to start).
 *
 * The start cell is treated specially: it has no parent but must be
 * the first cell in the result.  Reconstruction is done by following
 * parent links from end → start, then reversing the result.
 */
int path_reconstruct(const ParentGrid *pg,
                     Coord start, Coord end,
                     Coord path_out[], int max_len);

/* ------------------------------------------------------------------ */
/* Exploration trace                                                    */
/*                                                                     */
/* The trace records the order in which cells are first discovered     */
/* (i.e., first time they are added to the queue/stack).               */
/* ------------------------------------------------------------------ */
typedef struct {
    Coord cells[MAX_PATH_LEN];
    int   count;
} Trace;

void trace_init(Trace *t);

/**
 * Append a cell to the trace.
 * Returns 0 on success, -1 if the trace is full.
 */
int trace_add(Trace *t, Coord c);

#endif /* PATH_H */
