#include "queue.h"

void queue_init(Queue *q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

int queue_is_empty(const Queue *q) {
    return (q->count == 0);
}

int queue_is_full(const Queue *q) {
    return (q->count == QUEUE_CAPACITY);
}

int queue_enqueue(Queue *q, Coord c) {
    if (queue_is_full(q)) return -1;
    q->data[q->tail] = c;
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;  /* circular wrap */
    q->count++;
    return 0;
}

int queue_dequeue(Queue *q, Coord *out) {
    if (queue_is_empty(q)) return -1;
    *out   = q->data[q->head];
    q->head = (q->head + 1) % QUEUE_CAPACITY;  /* circular wrap */
    q->count--;
    return 0;
}

int queue_count(const Queue *q) {
    return q->count;
}
