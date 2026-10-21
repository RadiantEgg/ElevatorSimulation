#include <stdio.h>
#include <stdlib.h>

#include "elevator.h"
#include "queue.h"

static int any_request(const Elevator *e, const Floor floors[FLOOR_COUNT])
{
    for (int i = 0; i < FLOOR_COUNT; i++) {
        if (floors[i].up_lit == 1 || floors[i].down_lit == 1 || e->dest[i] == 1)
            return 1;
    }
    return 0;
}

/* 当前方向前面是否有请求 */
static int no_request_ahead(const Elevator *e, const Floor floors[FLOOR_COUNT])
{
    int step = (e->move == MOVE_UP) ? 1 : -1;

    for (int i = e->floor + step; i >= 0 && i < FLOOR_COUNT; i += step) {
        if (floors[i].up_lit == 1 || floors[i].down_lit == 1 || e->dest[i] == 1)
            return 0;
    }
    return 1;
}

static int check_person_appeared_floor(Floor floors[FLOOR_COUNT]) 
{
    for (int i = 0; i < FLOOR_COUNT; i++) 
        if (floors[i].up_lit == 1 || floors[i].down_lit == 1)
            return i;
    
    return -1;
}

static int elevator_is_serving(const Elevator *e, const Person *p)
{
    if (e->floor != p->appear_floor)
        return 0;
    if (e->move == MOVE_UP && p->direction != DIR_UP)
        return 0;
    if (e->move == MOVE_DOWN && p->direction != DIR_DOWN)
        return 0;
    if (e->move != MOVE_UP && e->move != MOVE_DOWN)
        return 0;

    switch (e->action.kind) {
        case ACT_UP_HALT:
        case ACT_DOWN_HALT:
        case ACT_OPEN:
        case ACT_EXIT:
        case ACT_ENTER:
        case ACT_WAIT:
        case ACT_CLOSE:
            return 1;
        default:
            return 0;
    }
}

// 电梯初始化，同时把整个楼的层数组初始化
void elevator_init(Elevator *e, Floor floors[FLOOR_COUNT])
{
    e->floor = HOME_FLOOR;
    e->move = MOVE_NONE;

    e->action.kind = ACT_IDLE;
    e->action.start = 0;
    e->action.finish = 0;

    for (int i = 0; i < FLOOR_COUNT; i++)
        e->dest[i] = 0;
    
    
    dlist_init(&e->cabin);

    for (int i = 0; i < FLOOR_COUNT; i++) {
        queue_init(&floors[i].up);
        queue_init(&floors[i].down);
        floors[i].up_lit = 0;
        floors[i].down_lit = 0;
    }
}

// 处理人放弃
void elevator_give_up(Elevator *e, Person people[], int id, int now)
{
    Person *p = &people[id];

    if (p->state != PERSON_WAITING)
        return;
    if (now <= p->giveup_time)
        return;
    if (elevator_is_serving(e, p))
        return;

    person_set_state(p, PERSON_GAVE_UP);
}

// 处理当前动作事件
/* 只改action，不改floor、move、dest、cabin */
void elevator_set_action(Elevator *e, ActionKind kind, int now, int duration, int person_id)
{
    e->action.kind = kind;
    e->action.start = now;
    e->action.finish = now + duration;
    e->action.person_id = person_id;        // 这边传入id是-1说明应该是和人无关的事件
}



// 处理出现的一个人，根据id去人数组里面找信息
/* 出现一个人调用一次arrive
    基本：进队，点灯，变为waiting 
    附加：人出现时刻当前action还没结束：用setaction换事件
*/

// 调后面那个decide的时候报错了，先声明一下
void elevator_decide_next(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int now);
void elevator_arrive(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int id, int now)
{
    Person *p = &people[id];
    Floor *f = &floors[p->appear_floor];
    Queue *person_appeared_queue = (p->direction == DIR_UP) ? &f->up : &f->down;
    int *lit_light = (p->direction == DIR_UP) ? &f->up_lit : &f->down_lit;


    // 进队、点灯、变waiting
    queue_enqueue(person_appeared_queue, id);
    *lit_light = 1;
    person_set_state(p, PERSON_WAITING);

    /* 后面接那个比较麻烦的不完整的事件，就是人来的时候打断wait、close、idle */
    /* 等不到action finish的替换 */

    if (e->action.kind == ACT_IDLE) {
        e->move = MOVE_NONE;
        // 这里马上改状态
        elevator_decide_next(e, floors, people, now);
        return;
    }

    if (e->move == MOVE_RETURN) {
        int next_floor_of_interrupted_action;
        if (e->action.kind == ACT_UP_FLOOR) {
            next_floor_of_interrupted_action = e->floor + 1;
        } else if (e->action.kind == ACT_DOWN_FLOOR) {
            next_floor_of_interrupted_action = e->floor - 1;
        } else {
            next_floor_of_interrupted_action = e->floor;
        }

        if (next_floor_of_interrupted_action < p->appear_floor) {
            e->move = MOVE_UP;
        } else if (next_floor_of_interrupted_action > p->appear_floor) {
            e->move = MOVE_DOWN;
        } else {
            e->move = (p->direction == DIR_UP) ? MOVE_UP : MOVE_DOWN;
        }
    }

    /* 没赶上 */
    if (!elevator_is_serving(e, p))
        return;
    if (e->action.kind == ACT_CLOSE && now >= e->action.finish)
        return;

    // 赶上了
    *lit_light = 0;
    if (e->action.kind == ACT_WAIT) {
        queue_dequeue(person_appeared_queue, &id);
        elevator_set_action(e, ACT_ENTER, now, T_ENTER, id);
        return;
    }

    if (e->action.kind == ACT_CLOSE) {
        int closed_time = now - e->action.start;

        elevator_set_action(e, ACT_OPEN, now, closed_time, -1);
    }
}



/* 
每个电梯行程由多个action组成，做完一个action之后用结果处理的函数更新状态，
然后再由决策函数根据当前状态以及整个楼的情况用set action选定下一个action。

而时钟是直接跳转到当前action的finish，然后处理结果，再决策
*/

// 处理这个事件做完之后的结果
/* sim内某个action finish时调用，只写动作结果，不改action本身 */
void elevator_finish_action(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int now)
{
    // 如果action和人有关先记下人
    Person *p;
    if (e->action.person_id >= 0) 
        p = &people[e->action.person_id];

    switch (e->action.kind) {

        // 上下楼只有楼层变化
        case ACT_UP_FLOOR:
            e->floor++;
            break;
        
        case ACT_DOWN_FLOOR:
            e->floor--;
            break;


        /* 决策函数peek之后，发现这个人是waiting状态，让人delete queue，所以set elevator action，把action设置为enter， 同时记下id。所以现在处理结果的情况是人不在队伍里面，也不在电梯内，只是电梯记住了事件是“人要进来”以及人的id。
            所以：1、处理这个结果时电梯已经知道当前事件是什么，也就是action和id知道
                    2、处理的结果是，action结束时人进入电梯（dlist），状态改变（state），以及按下对应目标楼层（dest）

            还有灯的事情，应该是电梯决定接人的时候直接灭，还没开始enter这个action，反向外呼留到最后那个复杂判断，还没决定
        */
        case ACT_ENTER:
            person_set_state(p, PERSON_IN_CABIN);
            dlist_append(&e->cabin, e->action.person_id);
            e->dest[p->target_floor] = 1;
            break;

        /* 
            结果是：
            1、人变为arrived
            从dlist里面删除是决策函数找到人的pre之后delete，然后才进入中间状态
            本层的dest的灯会灭，在电梯决定要下人的时候就灭，在exit之前
        */
        case ACT_EXIT:
            person_set_state(p, PERSON_ARRIVED);
            break;

    }
}

/* dest 的清除是在第一次exit之前，后面由decide反复从dlist里面倒序找目标楼层是这层的人，逐个设置exit action每个action结束之后重新decide */

/* 这两个函数在时钟里面调用，跳到finish的时候先finishaction，再elevatordecide一次 */


/* 在finish action之后调用一次，sim中用做完的action.kind调用set写下一个,里面存的action还是刚刚finish之后还没改的那个 */
/* 正常finish走decide，打斷走arrive */
static int will_stop(const Elevator *e, const Floor *floor, const Floor floors[FLOOR_COUNT])
{
    int same_lit = (e->move == MOVE_UP) ? floor->up_lit : floor->down_lit;
    int opposite_lit = (e->move == MOVE_UP) ? floor->down_lit : floor->up_lit;

    if (same_lit == 1 || e->dest[e->floor] == 1)
        return 1;
    if (opposite_lit == 1 && no_request_ahead(e, floors))
        return 1;
    return 0;
}

static int move_interrupted(Elevator *e)
{
    return (e->action.kind == ACT_UP_FLOOR && e->move == MOVE_DOWN || e->action.kind == ACT_DOWN_FLOOR && e->move == MOVE_UP) ? 1 : 0;
}

void elevator_decide_next(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int now)
{
    /* idle被打断，arrive设置为NONE，这里控制灯 */
    if (e->move == MOVE_NONE) {
        int preson_appered_floor = check_person_appeared_floor(floors);
        /* 没人出现, 记为idle，等待第一次打断 */
        if (preson_appered_floor == -1) {
            elevator_set_action(e, ACT_IDLE, now, T_IDLE, -1);
            return;
        }

        /* idle被打断，decide来控制move，lit */
        // 当前层
        Floor *cur_floor = &floors[e->floor];
        if (preson_appered_floor == e->floor) {
            int *lit_light = (cur_floor->up_lit == 1) ? &cur_floor->up_lit : &cur_floor->down_lit;

            e->move = (cur_floor->up_lit == 1) ? MOVE_UP : MOVE_DOWN;
            *lit_light = 0;
            elevator_set_action(e, ACT_OPEN, now, T_OPEN, -1);
            return;
        }

        // 其它层
        e->move = (preson_appered_floor > e->floor) ? MOVE_UP : MOVE_DOWN;
        elevator_set_action(e, ACT_ACCEL, now, T_ACCEL, -1);
        return;
    }

    
    int id;
    Floor *cur_floor = &floors[e->floor];

    switch (e->action.kind) {

        case ACT_ACCEL:
            /* 打断return */
            if (e->move == MOVE_RETURN) {
                if (e->floor > HOME_FLOOR) {
                    elevator_set_action(e, ACT_DOWN_FLOOR, now, T_DOWN_FLOOR, -1);
                    return;                    
                } else if (e->floor < HOME_FLOOR) {
                    elevator_set_action(e, ACT_UP_FLOOR, now, T_UP_FLOOR, -1);
                    return;      
                }
            }

            // 正常加速
            ActionKind floor_act = (e->move == MOVE_UP) ? ACT_UP_FLOOR : ACT_DOWN_FLOOR;
            int floor_time = (e->move == MOVE_UP) ? T_UP_FLOOR : T_DOWN_FLOOR;
            elevator_set_action(e, floor_act, now, floor_time, -1);
            return;
            


        case ACT_UP_FLOOR:
        case ACT_DOWN_FLOOR:
            /* 返程走到 1 楼且没有任何请求就不再向上。 */
            if (e->move == MOVE_RETURN && e->floor == HOME_FLOOR
                && !any_request(e, floors)) {
                e->move = MOVE_NONE;
                return;
            }
            
            /* 刚走完的一层和 move 相反 */
            if (move_interrupted(e)) {
                // 在这层
                if (cur_floor->up_lit == 1 || cur_floor->down_lit == 1) {
                    ActionKind halt_act = (e->action.kind == ACT_UP_FLOOR) ? ACT_UP_HALT : ACT_DOWN_HALT;
                    int halt_time = (e->action.kind == ACT_UP_FLOOR) ? T_UP_HALT : T_DOWN_HALT;
                    elevator_set_action(e, halt_act, now, halt_time, -1);
                    return;
                }
                // 不在这层
                elevator_set_action(e, ACT_ACCEL, now, T_ACCEL, -1);
                return;
            }
        
            if (will_stop(e, cur_floor, floors)) {
                // 要停
                ActionKind halt_act = (e->move == MOVE_UP) ? ACT_UP_HALT : ACT_DOWN_HALT;
                int halt_time = (e->move == MOVE_UP) ? T_UP_HALT : T_DOWN_HALT;

                elevator_set_action(e, halt_act, now, halt_time, -1);
                return;
            } else {
                // 不停
                ActionKind floor_act = (e->move == MOVE_UP) ? ACT_UP_FLOOR : ACT_DOWN_FLOOR;
                int floor_time = (e->move == MOVE_UP) ? T_UP_FLOOR : T_DOWN_FLOOR;

                elevator_set_action(e, floor_act, now, floor_time, -1);
                return;
            }
                



        case ACT_UP_HALT:
        case ACT_DOWN_HALT:
            // 要停下，所以两侧都灭灯
            int *lit_light = (e->action.kind == ACT_UP_HALT) ? &cur_floor->up_lit : &cur_floor->down_lit;

            e->dest[e->floor] = 0;
            *lit_light = 0;
            elevator_set_action(e, ACT_OPEN, now, T_OPEN, -1);
            return;


        case ACT_OPEN:
        case ACT_EXIT:
        case ACT_ENTER:
            /* exit部分 */
            DListNode *cur = dlist_tail(&e->cabin);
            
            while (cur != e->cabin.head) {
                DListNode *pre = cur->pre;
                if (people[cur->id].target_floor == e->floor) {
                    // 这里找到这层下的人就删掉
                    id = cur->id;
                    dlist_delete(&e->cabin, cur);
                    elevator_set_action(e, ACT_EXIT, now, T_EXIT, id); 
                    return;
                }
                cur = pre; 
            }
        
            /* enter部分 */
            QueueStatus st;
        
            // 停的时候已经灭灯了，所以这里不根据灯判断
            Queue *cur_queue = (e->move == MOVE_UP) ? &cur_floor->up : &cur_floor->down;

            while ((st = queue_peek(cur_queue, &id)) != QUEUE_EMPTY) {
                // 放弃就直接跳过
                if (people[id].state == PERSON_GAVE_UP) {
                    queue_dequeue(cur_queue, &id);
                    continue;
                }
                // 找到可以上的人
                if (people[id].state == PERSON_WAITING) {
                    queue_dequeue(cur_queue, &id);
                    elevator_set_action(e, ACT_ENTER, now, T_ENTER, id);
                    return;
                }
            }
            // 没人进

            /* wait部分 */
            elevator_set_action(e, ACT_WAIT, now, T_WAIT, -1);
            return;


        case ACT_WAIT:
            elevator_set_action(e, ACT_CLOSE, now, T_CLOSE, -1);
            return;

        // 这里是那个max的复杂部分，处理要不要换方向
        case ACT_CLOSE:
            // 关门之后前方还有请求
            int *opposite_light = (e->move == MOVE_UP) ? &cur_floor->down_lit : &cur_floor->up_lit;

            if (!no_request_ahead(e, floors)) {
                elevator_set_action(e, ACT_ACCEL, now, T_ACCEL, -1);
                return;
            }
                
            /* 这层同向外呼 */
            int *same_light = (e->move == MOVE_UP) ? &cur_floor->up_lit : &cur_floor->down_lit;
            if (*same_light == 1) {
                *same_light = 0;
                elevator_set_action(e, ACT_OPEN, now, T_OPEN, -1);
                return;
            }

            /* 前方五请求，当前层无同向外呼，有反向外呼 */
            if (*opposite_light == 1) {
                e->move = (e->move == MOVE_UP) ? MOVE_DOWN : MOVE_UP;
                *opposite_light = 0;
                elevator_set_action(e, ACT_OPEN, now, T_OPEN, -1);
                return;
            }

            /* 这层以及以上没有i请求，下方有请求 */
            if (any_request(e, floors)) {
                e->move = (e->move == MOVE_UP) ? MOVE_DOWN : MOVE_UP;
                elevator_set_action(e, ACT_ACCEL, now, T_ACCEL, -1);
                return;
            }

            /* 都没没有 */
            elevator_set_action(e, ACT_IDLE, now, T_IDLE, -1);
            return;

        case ACT_IDLE:
            if (e->floor == 1) {
                e->move = MOVE_NONE;
                return;
            }else if (e->floor > 1) {
                e->move = MOVE_RETURN;
                elevator_set_action(e, ACT_ACCEL, now, T_ACCEL , -1);
                return;
            } else {
                e->move = MOVE_RETURN;
                elevator_set_action(e, ACT_ACCEL, now, T_ACCEL, -1);
                return;               
            }

        
    }


}

