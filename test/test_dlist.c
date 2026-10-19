#include <stdio.h>

#include "dlist.h"

static int failed;

static const char *status_name(DListStatus status)
{
    switch (status) {
    case DLIST_OK:
        return "OK";
    case DLIST_NOMEM:
        return "NOMEM";
    default:
        return "?";
    }
}

static void expect_status(const char *step, DListStatus got, DListStatus want)
{
    if (got == want) {
        printf("  %s -> %s\n", step, status_name(got));
        return;
    }

    printf("  %s -> %s，约定是 %s\n", step, status_name(got), status_name(want));
    failed = 1;
}

static void expect_size(const char *step, DList *s, int want)
{
    int got = dlist_size(s);

    if (got == want) {
        printf("  %s -> %d\n", step, got);
        return;
    }

    printf("  %s -> %d，约定是 %d\n", step, got, want);
    failed = 1;
}

/* 没有人时，尾和头是同一个哨兵，人数是 0。不读哨兵的编号。 */
static void expect_empty(DList *s)
{
    expect_size("人数", s, 0);

    if (s->head == NULL) {
        printf("  头节点是空指针\n");
        failed = 1;
        return;
    }

    if (dlist_tail(s) == s->head)
        printf("  尾节点就是头节点\n");
    else {
        printf("  尾节点不是头节点\n");
        failed = 1;
    }

    if (s->head->next == NULL)
        printf("  头节点的后继是空指针\n");
    else {
        printf("  头节点的后继不是空指针\n");
        failed = 1;
    }
}

/* 从尾往头只看 size 个人。再往前应是头节点。 */
static DListNode *find_id(DList *s, int id)
{
    DListNode *node = dlist_tail(s);
    int count = dlist_size(s);

    for (int i = 0; i < count; i++) {
        if (node == NULL || node == s->head)
            return NULL;
        if (node->id == id)
            return node;
        node = node->pre;
    }

    return NULL;
}

static void expect_backward(DList *s, const int *ids, int count)
{
    DListNode *node = dlist_tail(s);

    if (dlist_size(s) == 0) {
        if (node == s->head)
            printf("  人数是 0，尾节点就是头节点\n");
        else {
            printf("  人数是 0，尾节点不是头节点\n");
            failed = 1;
        }
        return;
    }

    for (int i = 0; i < count; i++) {
        if (node == NULL || node == s->head) {
            printf("  倒序第 %d 个已经到头节点，约定编号是 %d\n", i + 1, ids[i]);
            failed = 1;
            return;
        }

        if (node->id == ids[i])
            printf("  倒序第 %d 个 -> %d\n", i + 1, node->id);
        else {
            printf("  倒序第 %d 个 -> %d，约定是 %d\n", i + 1, node->id, ids[i]);
            failed = 1;
        }

        if (i == 0) {
            if (node->next == NULL)
                printf("  尾的后继是空指针\n");
            else {
                printf("  尾的后继不是空指针\n");
                failed = 1;
            }
        } else if (node->next == NULL || node->next->pre != node) {
            printf("  编号 %d 和后继的链接不对\n", node->id);
            failed = 1;
        }

        if (i + 1 == count) {
            if (node->pre == s->head)
                printf("  再往前是头节点\n");
            else {
                printf("  再往前不是头节点\n");
                failed = 1;
            }
            if (s->head->next == node)
                printf("  头节点的后继是最早的编号 %d\n", ids[i]);
            else {
                printf("  头节点的后继不是最早的编号 %d\n", ids[i]);
                failed = 1;
            }
            return;
        }

        if (node->pre == NULL || node->pre == s->head || node->pre->next != node) {
            printf("  编号 %d 和前驱的链接不对\n", node->id);
            failed = 1;
            return;
        }

        node = node->pre;
    }
}

static void delete_id(DList *s, int id)
{
    DListNode *node = find_id(s, id);

    printf("摘掉编号 %d\n", id);
    fflush(stdout);

    if (node == NULL) {
        printf("  从尾往头没有找到这个编号\n");
        failed = 1;
        return;
    }

    dlist_delete(s, node);
}

static void append_ids(DList *s, const int *ids, int count)
{
    for (int i = 0; i < count; i++) {
        char step[32];

        snprintf(step, sizeof step, "append %d", ids[i]);
        expect_status(step, dlist_append(s, ids[i]), DLIST_OK);
    }
}

int main(void)
{
    DList list;
    const int first[] = {10, 20, 30};
    const int after_tail[] = {20, 10};
    const int after_middle[] = {30, 10};

    printf("初始化一个栈上的双向链表\n");
    dlist_init(&list);
    expect_empty(&list);

    printf("按 10、20、30 接到尾部\n");
    fflush(stdout);
    append_ids(&list, first, 3);
    expect_size("人数", &list, 3);
    {
        const int backward[] = {30, 20, 10};

        expect_backward(&list, backward, 3);
    }

    delete_id(&list, 30);
    expect_size("人数", &list, 2);
    expect_backward(&list, after_tail, 2);

    printf("补回 30，再摘掉中间的 20\n");
    fflush(stdout);
    expect_status("append 30", dlist_append(&list, 30), DLIST_OK);
    delete_id(&list, 20);
    expect_size("人数", &list, 2);
    expect_backward(&list, after_middle, 2);

    delete_id(&list, 10);
    expect_size("人数", &list, 1);
    {
        const int only[] = {30};

        expect_backward(&list, only, 1);
    }

    delete_id(&list, 30);
    printf("最后一个也摘掉\n");
    expect_empty(&list);

    printf("空链表后再接 40\n");
    fflush(stdout);
    expect_status("append 40", dlist_append(&list, 40), DLIST_OK);
    expect_size("人数", &list, 1);
    {
        const int again[] = {40};

        expect_backward(&list, again, 1);
    }

    printf("销毁\n");
    fflush(stdout);
    dlist_destroy(&list);

    if (failed) {
        printf("有步骤和约定不一致\n");
        return 1;
    }

    printf("全部步骤和约定一致\n");
    return 0;
}
