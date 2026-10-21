#ifndef STACK_H
#define STACK_H

#include "maze.h"

/* ------------------------------------------------------------------ */
/* Array-backed LIFO stack for iterative DFS                           */
/* ------------------------------------------------------------------ */

#define STACK_CAPACITY (MAZE_MAX_ROWS * MAZE_MAX_COLS)

typedef struct {
    Coord data[STACK_CAPACITY];
    int   top;   /* index of the top element; -1 when empty */
} Stack;

/** Initialise an empty stack. */
void stack_init(Stack *s);

/** Returns 1 if the stack is empty, 0 otherwise. */
int stack_is_empty(const Stack *s);

/** Returns 1 if the stack is full, 0 otherwise. */
int stack_is_full(const Stack *s);

/**
 * Push a coordinate.
 * Returns 0 on success, -1 if the stack is full.
 */
int stack_push(Stack *s, Coord c);

/**
 * Pop the top coordinate into *out.
 * Returns 0 on success, -1 if the stack is empty.
 */
int stack_pop(Stack *s, Coord *out);

/**
 * Peek at the top without removing it.
 * Returns 0 on success, -1 if the stack is empty.
 */
int stack_peek(const Stack *s, Coord *out);

/** Return the number of elements currently in the stack. */
int stack_count(const Stack *s);

#endif /* STACK_H */
