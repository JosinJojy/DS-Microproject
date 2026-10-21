#ifndef MAZE_H
#define MAZE_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Constants                                                            */
/* ------------------------------------------------------------------ */
#define MAZE_MAX_ROWS  51
#define MAZE_MAX_COLS  51
#define MAZE_MIN_ROWS   3
#define MAZE_MIN_COLS   3

/* Cell state values stored in the grid */
typedef enum {
    CELL_EMPTY = 0,   /* traversable open cell */
    CELL_WALL  = 1    /* impassable wall */
} CellState;

/* A single grid coordinate */
typedef struct {
    int row;
    int col;
} Coord;

/* The maze structure */
typedef struct {
    int       rows;
    int       cols;
    uint8_t   grid[MAZE_MAX_ROWS][MAZE_MAX_COLS]; /* CellState values */
    Coord     start;
    Coord     end;
} Maze;

/* ------------------------------------------------------------------ */
/* Functions                                                            */
/* ------------------------------------------------------------------ */

/**
 * Initialise a maze to all-empty cells with start=(0,0) and end=(rows-1,cols-1).
 * Returns 0 on success, -1 if dimensions are out of range.
 */
int maze_init(Maze *m, int rows, int cols);

/**
 * Returns 1 if (row, col) is within bounds, 0 otherwise.
 */
int maze_in_bounds(const Maze *m, int row, int col);

/**
 * Returns 1 if coord is within bounds AND the cell is CELL_EMPTY, 0 otherwise.
 */
int maze_is_traversable(const Maze *m, Coord c);

/**
 * Returns 1 if the two coords are orthogonally adjacent (differ by exactly
 * one step in row or col, not both).
 */
int maze_adjacent(Coord a, Coord b);

/**
 * Validate a fully specified maze (dimensions, grid values, start, end).
 * Returns 0 if valid, or a negative error code:
 *  -1  invalid dimensions
 *  -2  grid contains invalid cell values
 *  -3  start out of bounds or on a wall
 *  -4  end out of bounds or on a wall
 */
int maze_validate(const Maze *m);

/**
 * Fill maze from a 2-D array of ints (0=empty, 1=wall).
 * Returns 0 on success, -1 on invalid value.
 */
int maze_load_grid(Maze *m, const int flat[], int rows, int cols);

#endif /* MAZE_H */
