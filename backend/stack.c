#include "stack.h"

void stack_init(Stack *s) {
    s->top = -1;
}

int stack_is_empty(const Stack *s) {
    return (s->top < 0);
}

int stack_is_full(const Stack *s) {
    return (s->top >= STACK_CAPACITY - 1);
}

int stack_push(Stack *s, Coord c) {
    if (stack_is_full(s)) return -1;
    s->data[++(s->top)] = c;
    return 0;
}

int stack_pop(Stack *s, Coord *out) {
    if (stack_is_empty(s)) return -1;
    *out = s->data[(s->top)--];
    return 0;
}

int stack_peek(const Stack *s, Coord *out) {
    if (stack_is_empty(s)) return -1;
    *out = s->data[s->top];
    return 0;
}

int stack_count(const Stack *s) {
    return s->top + 1;
}
