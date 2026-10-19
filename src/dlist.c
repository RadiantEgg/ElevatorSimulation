#include <stdio.h>
#include <stdlib.h>

#include "dlist.h"

static DListNode *make_dlistnode(int id)
{
    DListNode *newnode = malloc(sizeof(newnode));
    if (newnode == NULL)    return NULL;

    newnode->id = id;
    newnode->pre = NULL;
    newnode->next = NULL;
    return newnode;
}

void dlist_init(DList *s)
{
    s->head = malloc(sizeof(DListNode));
    if (s->head == NULL)    return;

    s->head->next = NULL;
    s->tail = s->head;
    s->size = 0;
}

void dlist_destroy(DList *s)
{
    if (s == NULL)      return;

    DListNode *cur = s->head->next;
    free(s->head);
    s->head = NULL;

    while (cur) {
        DListNode *tmp = cur;
        cur = tmp->next;
        free(tmp);
    }

    s->tail = s->head;
    s->size = 0;
}

DListStatus dlist_append(DList *s, int id)
{
    DListNode *newnode = make_dlistnode(id);
    if (newnode == NULL)    return DLIST_NOMEM;

    s->tail->next = newnode;
    newnode->pre = s->tail;
    
    s->tail = newnode;
    s->size++;

    return DLIST_OK;
}

void dlist_delete(DList *s, DListNode *node)
{
    if (s->size == 0)   return;

    if (node == s->tail) {
        s->tail = node->pre;
        s->tail->next = NULL;
        free(node);
        s->size--;
    } else {
        DListNode *pre_node = node->pre;
        pre_node->next = node->next;
        node->next->pre = pre_node;
        free(node);
        s->size--;
    }
}

DListNode *dlist_tail(DList *s)
{
    return s->tail;
}

int dlist_size(DList *s)
{
    return s->size;
}