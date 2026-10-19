#ifndef PERSON_H
#define PERSON_H

#include "const.h"

typedef enum {
    PERSON_NOT_APPEARED,
    PERSON_WAITING,
    PERSON_IN_CABIN,
    PERSON_ARRIVED,
    PERSON_GAVE_UP,
} PersonState;

typedef struct {
    int appear_time;
    int appear_floor;
    int target_floor;
    int giveup_time;
    Direction direction;
    PersonState state;
} Person;

void person_init(Person *p, int appear_time, int appear_floor, int target_floor, int max_wait);
void person_set_state(Person *p, PersonState state);

#endif