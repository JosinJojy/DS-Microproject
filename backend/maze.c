#include "maze.h"
#include <string.h>

int maze_init(Maze *m, int rows, int cols) {
    if (rows < MAZE_MIN_ROWS || rows > MAZE_MAX_ROWS ||
        cols < MAZE_MIN_COLS || cols > MAZE_MAX_COLS) {
        return -1;
    }
    m->rows = rows;
    m->cols = cols;
    memset(m->grid, CELL_EMPTY, sizeof(m->grid));
    m->start.row = 0;
    m->start.col = 0;
    m->end.row   = rows - 1;
    m->end.col   = cols - 1;
    return 0;
}

int maze_in_bounds(const Maze *m, int row, int col) {
    return (row >= 0 && row < m->rows && col >= 0 && col < m->cols);
}

int maze_is_traversable(const Maze *m, Coord c) {
    if (!maze_in_bounds(m, c.row, c.col)) return 0;
    return (m->grid[c.row][c.col] == CELL_EMPTY);
}

int maze_adjacent(Coord a, Coord b) {
    int dr = a.row - b.row;
    int dc = a.col - b.col;
    /* Exactly one step in one direction */
    return ((dr == 1 || dr == -1) && dc == 0) ||
           (dr == 0 && (dc == 1 || dc == -1));
}

int maze_validate(const Maze *m) {
    if (m->rows < MAZE_MIN_ROWS || m->rows > MAZE_MAX_ROWS ||
        m->cols < MAZE_MIN_COLS || m->cols > MAZE_MAX_COLS) {
        return -1;
    }
    /* Check all cell values */
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            if (m->grid[r][c] != CELL_EMPTY && m->grid[r][c] != CELL_WALL) {
                return -2;
            }
        }
    }
    /* Validate start */
    if (!maze_in_bounds(m, m->start.row, m->start.col) ||
        m->grid[m->start.row][m->start.col] == CELL_WALL) {
        return -3;
    }
    /* Validate end */
    if (!maze_in_bounds(m, m->end.row, m->end.col) ||
        m->grid[m->end.row][m->end.col] == CELL_WALL) {
        return -4;
    }
    return 0;
}

int maze_load_grid(Maze *m, const int flat[], int rows, int cols) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int v = flat[r * cols + c];
            if (v != CELL_EMPTY && v != CELL_WALL) return -1;
            m->grid[r][c] = (uint8_t)v;
        }
    }
    return 0;
}
