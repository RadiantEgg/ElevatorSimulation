#include <stdio.h>
#include <stdlib.h>

#include "person.h"

static Direction get_direction(Person *p)
{
    return (p->appear_floor < p->target_floor) ? DIR_UP : DIR_DOWN;
}

void person_init(Person *p, int appear_time, int appear_floor, int target_floor, int max_wait)
{
    p->appear_time = appear_time;
    p->appear_floor = appear_floor;
    p->target_floor = target_floor;
    p->giveup_time = p->appear_time + max_wait;

    p->direction = get_direction(p);
    p->state = PERSON_NOT_APPEARED;
}

void person_set_state(Person *p, PersonState state)
{
    p->state = state;
}