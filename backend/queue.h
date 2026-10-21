#ifndef QUEUE_H
#define QUEUE_H

#include "maze.h"

/* ------------------------------------------------------------------ */
/* Circular array FIFO queue for BFS                                   */
/*                                                                     */
/* Maximum capacity is one cell per grid position. Because the maximum */
/* grid is MAZE_MAX_ROWS × MAZE_MAX_COLS we size the queue accordingly. */
/* ------------------------------------------------------------------ */

#define QUEUE_CAPACITY (MAZE_MAX_ROWS * MAZE_MAX_COLS)

typedef struct {
    Coord  data[QUEUE_CAPACITY];
    int    head;   /* index of the front element        */
    int    tail;   /* index of the next write position  */
    int    count;  /* number of elements currently held */
} Queue;

/** Initialise an empty queue. */
void queue_init(Queue *q);

/** Returns 1 if the queue is empty, 0 otherwise. */
int queue_is_empty(const Queue *q);

/** Returns 1 if the queue is full, 0 otherwise. */
int queue_is_full(const Queue *q);

/**
 * Enqueue a coordinate.
 * Returns 0 on success, -1 if the queue is full.
 */
int queue_enqueue(Queue *q, Coord c);

/**
 * Dequeue a coordinate into *out.
 * Returns 0 on success, -1 if the queue is empty.
 */
int queue_dequeue(Queue *q, Coord *out);

/** Return the number of elements currently in the queue. */
int queue_count(const Queue *q);

#endif /* QUEUE_H */
