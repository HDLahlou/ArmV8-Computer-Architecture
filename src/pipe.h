/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>

typedef struct Pipe_Reg_IFtoDE_struct  {
// pc counter
uint64_t PC;

uint32_t instructionCode;

// predicted pc
uint64_t predicted_PC;

// b2b miss check
int b2b_missFlag;

} Pipe_Reg_IFtoDE_struct;

/* global variable for holding intermediate registers and information for Fetch to Decode*/
typedef struct Pipe_Reg_DEtoEX_struct  {
// instruction name
char instructionName[10];


// to branch flag
int branchFlag;

// pc counter
uint64_t PC;

// Determines if it will write to memory or not
int writeFlag;

// Determines if it will read from memory or not
int readFlag;


// Determines if flag needs to be updated or not
int setFlagCheck;

// holding Rm
int64_t Reg1;

// holding Rn
int64_t Reg2;

// holding Rd
int64_t Reg3;

// holding Imm1
int64_t Reg4;

// holding Imm2
int64_t Reg5;

// holding register number of Rm
int64_t Reg6;

// holding register number of Rn
int64_t Reg7;

// holding register number of Rd
int64_t Reg8;

// predicted pc
uint64_t predicted_PC;

// b2b miss check
int b2b_missFlag;

} Pipe_Reg_DEtoEX_struct;



// TODO: Use write and read flag to determine what we do in mem stage
// TODO: If writeFlag == True we do nothing in wb phase
// TODO: Pass writeFlag and ReadFlag from DE-EX-> EX-MEM -> MEM-WB
typedef struct Pipe_Reg_EXtoMEM_struct  {

    // instruction name
    char instructionName[10];

    // toBranch flag
    int branchFlag;

    // Determines if flag needs to be updated or not
    int setFlagCheck;
   
    //N Flag set
    int NFlag;

    //Z Flag set
    int ZFlag;

    // pc counter
    uint64_t PC;

    // Determines if it will write to memory or not
    int writeFlag;

    // Determines if it will read from memory or not
    int readFlag;

    // holding Rm (Only used in stur)
    int64_t Reg1;

    // holding Result (which goes to Rd)
    int64_t Reg3;

    // holding size
    int64_t Reg5;

    // holding register number of Rm
    int64_t Reg6;

    // holding register number of Rd
    int64_t Reg8;

} Pipe_Reg_EXtoMEM_struct;


// global struct holding intermediate register values for MEM TO WRITEBACK
typedef struct Pipe_Reg_MEMtoWB_struct  {

    // instruction name
    char instructionName[10];

    // to branch flag
    int branchFlag;

    // Determines if flag needs to be updated or not
    int setFlagCheck;


    // pc counter
    uint64_t PC;


    //N Flag set
    int NFlag;

    //Z Flag set
    int ZFlag;


    // Determines if it will write to memory or not
    int writeFlag;

    // Determines if it will read from memory or not
    int readFlag;

    // holding Rm (Only used in stur)
    //int64_t Reg1;

    // holding Result (which goes to Rd)
    int64_t Reg3;

    // holding size
    //int64_t Reg5;

    // holding register number of Rm
    int64_t Reg6;

    // holding register number of Rd
    int64_t Reg8;

} Pipe_Reg_MEMtoWB_struct;









typedef struct CPU_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;

} CPU_State;

int RUN_BIT;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

extern Pipe_Reg_DEtoEX_struct Pipe_Reg_DEtoEX;
extern Pipe_Reg_EXtoMEM_struct Pipe_Reg_EXtoMEM;
extern Pipe_Reg_MEMtoWB_struct Pipe_Reg_MEMtoWB;
extern Pipe_Reg_IFtoDE_struct Pipe_Reg_IFtoDE;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();



#endif
