#include <stdio.h>

#include "queue.h"

static int failed;

static const char *status_name(QueueStatus status)
{
    switch (status) {
    case QUEUE_OK:
        return "OK";
    case QUEUE_EMPTY:
        return "EMPTY";
    case QUEUE_NOMEM:
        return "NOMEM";
    default:
        return "?";
    }
}

static void expect_status(const char *step, QueueStatus got, QueueStatus want)
{
    if (got == want) {
        printf("  %s -> %s\n", step, status_name(got));
        return;
    }

    printf("  %s -> %s，约定是 %s\n", step, status_name(got), status_name(want));
    failed = 1;
}

static void expect_id(const char *step, int got, int want)
{
    if (got == want) {
        printf("  %s -> %d\n", step, got);
        return;
    }

    printf("  %s -> %d，约定是 %d\n", step, got, want);
    failed = 1;
}

static void expect_size(const char *step, const Queue *q, int want)
{
    if (q->size == want) {
        printf("  %s -> %d\n", step, q->size);
        return;
    }

    printf("  %s -> %d，约定是 %d\n", step, q->size, want);
    failed = 1;
}

/* 空队列时不应改写调用方的 id。先放一个明显的数，回来仍应是它。 */
static void check_empty(Queue *q, const char *title)
{
    int id = 111;
    QueueStatus status;

    printf("%s\n", title);

    status = queue_peek(q, &id);
    expect_status("peek", status, QUEUE_EMPTY);
    expect_id("peek 之后的 id", id, 111);

    id = 111;
    status = queue_dequeue(q, &id);
    expect_status("dequeue", status, QUEUE_EMPTY);
    expect_id("dequeue 之后的 id", id, 111);
    expect_size("个数", q, 0);
}

static void check_fifo(Queue *q)
{
    int id = 111;
    QueueStatus status;

    printf("按 10、20、30 进队，先进入的先出来\n");

    status = queue_enqueue(q, 10);
    expect_status("enqueue 10", status, QUEUE_OK);
    status = queue_enqueue(q, 20);
    expect_status("enqueue 20", status, QUEUE_OK);
    status = queue_enqueue(q, 30);
    expect_status("enqueue 30", status, QUEUE_OK);
    expect_size("个数", q, 3);

    status = queue_peek(q, &id);
    expect_status("peek", status, QUEUE_OK);
    expect_id("peek 到的编号", id, 10);
    expect_size("peek 不出队，个数", q, 3);

    status = queue_dequeue(q, &id);
    expect_status("dequeue", status, QUEUE_OK);
    expect_id("第一次出队", id, 10);
    expect_size("个数", q, 2);

    status = queue_peek(q, &id);
    expect_status("peek", status, QUEUE_OK);
    expect_id("peek 到的编号", id, 20);

    status = queue_dequeue(q, &id);
    expect_status("dequeue", status, QUEUE_OK);
    expect_id("第二次出队", id, 20);

    status = queue_dequeue(q, &id);
    expect_status("dequeue", status, QUEUE_OK);
    expect_id("第三次出队", id, 30);
    expect_size("个数", q, 0);
}

int main(void)
{
    Queue q;

    printf("初始化一个栈上的队列\n");
    queue_init(&q);
    expect_size("个数", &q, 0);

    check_empty(&q, "刚初始化，队列里没有编号");
    check_fifo(&q);

    printf("出空之后再进 40\n");
    fflush(stdout);
    expect_status("enqueue 40", queue_enqueue(&q, 40), QUEUE_OK);
    expect_size("个数", &q, 1);
    {
        int id = 111;
        expect_status("peek", queue_peek(&q, &id), QUEUE_OK);
        expect_id("peek 到的编号", id, 40);
        expect_status("dequeue", queue_dequeue(&q, &id), QUEUE_OK);
        expect_id("出队", id, 40);
    }

    check_empty(&q, "再次出空");

    printf("销毁。Queue 本身在栈上，只应释放节点\n");
    fflush(stdout);
    queue_destroy(&q);

    if (failed) {
        printf("有步骤和约定不一致\n");
        return 1;
    }

    printf("全部步骤和约定一致\n");
    return 0;
}
