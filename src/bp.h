/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 */

#ifndef _BP_H_
#define _BP_H_

#include "stdbool.h"
#include <limits.h>
#include "shell.h"


uint64_t getPC_bits(int x, int y, uint64_t PC);

// bp_predict result
uint64_t bp_predict_result;

int b2b_miss_check;

typedef struct b2b_entry{
    // pc
    uint64_t PC;

    // valid bit
    int valid_bit;

    // cond bit
    int cond_bit;

    // PC target
    uint64_t PC_target;


} b2b_entry;




typedef struct bp_t
{
    /* gshare */
    uint8_t ghr;

    /* PHT */
    int PHT[256];

    /* BTB */
    b2b_entry B2B[1024];


} bp_t;

extern bp_t bp_t_object;
extern uint64_t bp_predict_result;
extern int b2b_miss_check;

void bp_predict(uint64_t PC);
void bp_update(int condFlag, int takenFlag, uint64_t PC, uint64_t target);

#endif
