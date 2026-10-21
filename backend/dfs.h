#ifndef DFS_H
#define DFS_H

#include "maze.h"
#include "path.h"

/* ------------------------------------------------------------------ */
/* DFS result                                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    int   found;
    Trace trace;          /* exploration order (first discovery order) */
    Coord path[MAX_PATH_LEN];
    int   path_len;
} DFSResult;

/**
 * Run iterative DFS on the maze from m->start to m->end.
 *
 * Neighbor order (deterministic, reversed so the stack produces the
 * same effective expansion order as the BFS direction array):
 *   Pushed last → processed first.
 *   Push order: Left, Down, Right, Up  (so Up is processed first)
 *
 * A cell is added to the trace when it is first popped and confirmed
 * as unvisited (i.e., at the time it is actually explored).
 * The start cell is always the first trace entry.
 *
 * DFS does not guarantee a shortest path.
 * The returned path is valid (connected, wall-free, start-to-end).
 *
 * Returns 0 on success, -1 on internal error.
 */
int dfs_solve(const Maze *m, DFSResult *result);

#endif /* DFS_H */
