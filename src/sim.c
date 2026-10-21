#include <stdio.h>
#include <stdlib.h>

#include "elevator.h"
#include "person.h"

/* 每 1t 出现下一个人的概率是 20/1000 间隔正整数 */
#define APPEAR_PER_THOUSAND 20
#define DEFAULT_END 2000
#define DEFAULT_SEED 1

// 后面模拟的时候用
static const char *action_name(ActionKind kind)
{
    switch (kind) {
        case ACT_OPEN:
            return "开门";
        case ACT_CLOSE:
            return "关门";
        case ACT_ENTER:
            return "进门";
        case ACT_EXIT:
            return "出门";
        case ACT_WAIT:
            return "等待";
        case ACT_ACCEL:
            return "加速";
        case ACT_UP_FLOOR:
            return "向上一层";
        case ACT_DOWN_FLOOR:
            return "向下一层";
        case ACT_UP_HALT:
            return "上行临停";
        case ACT_DOWN_HALT:
            return "下行临停";
        case ACT_IDLE:
            return "空等";
        default:
            return "未知";
    }
}

// 算几何间隔
static int geometric_gap(void)
{
    int gap = 1;

    while ((rand() % 1000) >= APPEAR_PER_THOUSAND)
        gap++;
    return gap;
}

// 算目标楼层，不和原来的重复
static int other_floor(int appear)
{
    int target = rand() % (FLOOR_COUNT - 1);

    if (target >= appear)
        target++;
    return target;
}

// 一次性生成所有的人
static Person *generate_people(int end, int *count)
{
    Person *people = NULL;
    int n = 0;
    int t = 0;

    while (1) {
        int appear;
        int target;
        Person *grown;

        t += geometric_gap();
        if (t > end)
            break;

        appear = rand() % FLOOR_COUNT;
        target = other_floor(appear);
        grown = realloc(people, (n + 1) * sizeof *people);
        if (grown == NULL) {
            free(people);
            *count = 0;
            return NULL;
        }
        people = grown;
        // 初始化刷新出来的这个人，有那个随机等待时间400-1200
        person_init(&people[n], t, appear, target, 400 + rand() % 801);
        n++;
    }

    *count = n;
    return people;
}

// 记录人出现
static void log_appear(const Person *p, int id)
{
    printf("%d 人%d 出现在%d楼，去%d楼，%s，放弃时刻 %d\n",
           p->appear_time, id, p->appear_floor, p->target_floor,
           (p->direction == DIR_UP) ? "向上" : "向下", p->giveup_time);
}

// 记录电梯actionfinish一个
static void log_finish(int now, ActionKind kind, int person_id, int floor)
{
    printf("%d 电梯 %s结束", now, action_name(kind));
    // 和人有关再记录
    if (kind == ACT_ENTER || kind == ACT_EXIT)
        printf("，人%d", person_id);
    if (kind == ACT_UP_FLOOR || kind == ACT_DOWN_FLOOR)
        printf("，到达%d楼", floor);
    printf("\n");
}

// 下一个动作decide的那个
static void log_schedule(const Elevator *e)
{
    printf("%d 电梯 在%d楼 %s", e->action.start, e->floor, action_name(e->action.kind));
    if (e->action.person_id >= 0)
        printf(" 人%d", e->action.person_id);
    printf("，到 %d\n", e->action.finish);
}

// 队伍里面最早放弃的人
static int next_giveup(const Person people[], const int skip[], int count, int now)
{
    int best = -1;
    int i;

    for (i = 0; i < count; i++) {
        int t;

        if (skip[i] || people[i].state != PERSON_WAITING)
            continue;
        t = people[i].giveup_time + 1;
        if (t < now)
            continue;
        if (best < 0 || t < best)
            best = t;
    }
    return best;
}

// 三分记录里面挑最早的那个跳过去
static int earlier(int best, int candidate)
{
    if (candidate < 0)
        return best;
    if (best < 0 || candidate < best)
        return candidate;
    return best;
}

// 模拟结束的时候清掉所有前面分配的
static void destroy_building(Elevator *e, Floor floors[])
{
    int i;

    dlist_destroy(&e->cabin);
    for (i = 0; i < FLOOR_COUNT; i++) {
        queue_destroy(&floors[i].up);
        queue_destroy(&floors[i].down);
    }
}

int main(int argc, char **argv)
{
    int end = DEFAULT_END;
    unsigned seed = DEFAULT_SEED;

    Person *people;
    int count = 0;
    int *skip = NULL;
    int next_appear = 0;    // 下一个还没刷新的人的号码
    int now = 0;
    int steps = 0;
    Elevator elevator;
    Floor floors[FLOOR_COUNT];

    if (argc >= 2)
        end = atoi(argv[1]);
    if (argc == 3)
        seed = (unsigned)atoi(argv[2]);

    srand(seed);
    // 生成人
    people = generate_people(end, &count);
    if (count > 0 && people == NULL) {
        printf("生成人失败\n");
        return 1;
    }

    if (count > 0) {
        // 开始都是没跳过的
        skip = calloc(count, sizeof *skip);
        if (skip == NULL) {
            free(people);
            printf("生成人失败\n");
            return 1;
        }
    }

    elevator_init(&elevator, floors);
    printf("结束时刻 %d，种子 %u，出现 %d 人\n", end, seed, count);

    while (now <= end && steps < 100000) {
        // 都出现过记为-1
        int t_appear = next_appear < count ? people[next_appear].appear_time : -1;
        // 排队的里面没有skip的最早的
        int t_give = next_giveup(people, skip, count, now);
        int t_action = elevator.action.finish >= now ? elevator.action.finish : -1;
        int next = earlier(earlier(t_appear, t_give), t_action);
        int i;

        if (next < 0 || next > end)
            break;
        now = next;
        steps++;

        // 先处理人
        while (next_appear < count && people[next_appear].appear_time == now) {
            int finish_before = elevator.action.finish;

            log_appear(&people[next_appear], next_appear);
            elevator_arrive(&elevator, floors, people, next_appear, now);
            // 这里说明arrive里面打断了某个action
            if (elevator.action.finish != finish_before)
                log_schedule(&elevator);
            next_appear++;
        }

        // 处理放弃的
        for (i = 0; i < count; i++) {
            if (skip[i] || people[i].state != PERSON_WAITING)
                continue;
            if (people[i].giveup_time + 1 != now)
                continue;
            elevator_give_up(&elevator, people, i, now);
            if (people[i].state == PERSON_GAVE_UP)
                printf("%d 人%d 放弃\n", now, i);
            else
                skip[i] = 1;
        }

        // 处理电梯action
        if (elevator.action.finish == now) {
            ActionKind kind = elevator.action.kind;
            int person_id = elevator.action.person_id;
            int finish_before = elevator.action.finish;

            elevator_finish_action(&elevator, floors, people, now);
            log_finish(now, kind, person_id, elevator.floor);
            elevator_decide_next(&elevator, floors, people, now);
            if (elevator.action.finish != finish_before)
                log_schedule(&elevator);
        }
    }

    destroy_building(&elevator, floors);
    free(skip);
    free(people);
    return 0;
}
