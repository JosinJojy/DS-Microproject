#ifndef BFS_H
#define BFS_H

#include "maze.h"
#include "path.h"

/* ------------------------------------------------------------------ */
/* BFS result                                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    int   found;          /* 1 if path was found, 0 otherwise          */
    Trace trace;          /* exploration order (first discovery order)  */
    Coord path[MAX_PATH_LEN];
    int   path_len;       /* number of cells in the path               */
} BFSResult;

/**
 * Run BFS on the maze from m->start to m->end.
 *
 * Neighbor order (deterministic, clockwise from north):
 *   Up (-1, 0), Right (0, +1), Down (+1, 0), Left (0, -1)
 *
 * A cell is added to the trace the first time it is enqueued (discovered).
 * The start cell is always the first entry in the trace.
 *
 * Returns 0 on success (result populated), -1 on internal error.
 */
int bfs_solve(const Maze *m, BFSResult *result);

#endif /* BFS_H */
