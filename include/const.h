#ifndef CONST_H
#define CONST_H

#define FLOOR_COUNT 5
#define HOME_FLOOR 1

#define T_OPEN 20
#define T_CLOSE 20
#define T_ENTER 25
#define T_EXIT 25
#define T_WAIT 40
#define T_ACCEL 15
#define T_UP_FLOOR 51
#define T_DOWN_FLOOR 61
#define T_UP_HALT 14
#define T_DOWN_HALT 23
#define T_IDLE 300

typedef enum {
    DIR_UP,
    DIR_DOWN,
} Direction;

typedef enum {
    MOVE_NONE,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_RETURN,
} MoveState;

#endif