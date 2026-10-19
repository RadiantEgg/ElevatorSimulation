#ifndef DLIST_H
#define DLIST_H

typedef enum {
    DLIST_OK = 0,
    DLIST_NOMEM = 1,
} DListStatus;

typedef struct DListNode {
    int id;
    struct DListNode *pre;
    struct DListNode *next;
} DListNode;

typedef struct {
    DListNode *head;
    DListNode *tail;
    int size;
} DList;

void dlist_init(DList *s);
void dlist_destroy(DList *s);

DListStatus dlist_append(DList *s, int id);
void dlist_delete(DList *s, DListNode *node);

DListNode *dlist_tail(DList *s);
int dlist_size(DList *s);


#endif