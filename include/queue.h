#ifndef QUEUE_H
#define QUEUE_H

typedef enum {
    QUEUE_OK = 0,
    QUEUE_EMPTY = 1,
    QUEUE_NOMEM = 2,
} QueueStatus;

typedef struct QueueNode {
    int id;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *front;
    QueueNode *rear;
    int size;
} Queue;

void queue_init(Queue *q);
void queue_destroy(Queue *q);

QueueStatus queue_enqueue(Queue *q, int id);
QueueStatus queue_dequeue(Queue *q, int *id);
QueueStatus queue_peek(const Queue *q, int *id);


#endif