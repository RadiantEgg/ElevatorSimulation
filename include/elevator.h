#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "const.h"
#include "queue.h"
#include "dlist.h"
#include "person.h"

typedef enum {
    ACT_OPEN,
    ACT_CLOSE,
    ACT_ENTER,
    ACT_EXIT,
    ACT_WAIT,
    ACT_ACCEL,
    ACT_UP_FLOOR,
    ACT_DOWN_FLOOR,
    ACT_UP_HALT,
    ACT_DOWN_HALT,
    ACT_IDLE,
} ActionKind;

typedef struct {
    ActionKind kind;
    int start;
    int finish;
    int person_id;   // enter or exit 
} Action;

typedef struct {
    Queue up;
    Queue down;
    int up_lit;
    int down_lit;
} Floor;

typedef struct {
    int floor;      // current floor
    MoveState move;
    Action action;
    int dest[FLOOR_COUNT];
    DList cabin;
} Elevator;

// 只改数据，不做决定的函数
void elevator_init(Elevator *e, Floor floors[FLOOR_COUNT]);
void elevator_arrive(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int id, int now);
void elevator_give_up(Elevator *e, Person people[], int id, int now);
void elevator_set_action(Elevator *e, ActionKind kind, int now, int duration, int person_id);
void elevator_finish_action(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int now);
void elevator_decide_next(Elevator *e, Floor floors[FLOOR_COUNT], Person people[], int now);



#endif