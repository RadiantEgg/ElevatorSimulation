#include <stdio.h>
#include <stdlib.h>

#include "queue.h"


static QueueNode *make_queuenode(int id)
{
    QueueNode *newnode = malloc(sizeof(QueueNode));
    if (newnode == NULL)        return NULL;

    newnode->id = id;
    newnode->next = NULL;
    return newnode;
}

void queue_init(Queue *q)
{
    if (q == NULL)          return;

    q->front = malloc(sizeof(QueueNode));
    if (q->front == NULL)   return;

    q->front->id = -1;
    q->front->next = NULL;

    q->rear = q->front;
    q->size = 0;
}

void queue_destroy(Queue *q)
{
    if (q == NULL)      return;

    QueueNode *cur = q->front->next;
    free(q->front);
    q->front = NULL;

    while (cur) {
        QueueNode *tmp = cur;
        cur = cur->next;
        free(tmp);
    }

    q->rear = q->front;
    q->size = 0;
    // free(q);  这里不free，交给调用方
}


QueueStatus queue_enqueue(Queue *q, int id)
{
    QueueNode *newnode = make_queuenode(id);
    if (newnode == NULL)    return QUEUE_NOMEM;

    q->rear->next = newnode;
    newnode->next = NULL;
    q->rear = newnode;
    q->size += 1;
    return QUEUE_OK;
}

QueueStatus queue_dequeue(Queue *q, int *id)
{
    if (q->size == 0)       return QUEUE_EMPTY;

    QueueNode *delete_node = q->front->next;
    *id = delete_node->id;

    q->front->next = delete_node->next;
    q->size -= 1;

    if (q->size == 0)
        q->rear = q->front;

    free(delete_node);
    return QUEUE_OK;
}

QueueStatus queue_peek(const Queue *q, int *id)
{
    if (q->size == 0)   return QUEUE_EMPTY;
    
    *id = q->front->next->id;
    return QUEUE_OK;
}
