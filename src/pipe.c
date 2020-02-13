// Hakim Lahlou - hdlahlou
// Azam Mohsin - aamohsin
// FINAL VERSION
// edited nov 28 final version
/*
* CMSC 22200
*
* ARM pipeline timing simulator
*/

#include "pipe.h"
#include "shell.h"
#include "bp.h"
#include "cache.h"
#include "math.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


// instruction cache
cache_t *instructionCache;

// data cache
cache_t *dataCache;

// flag for if miss in instruction cache
int instructionCacheMissFlag = 0;
int dataCacheMissFlag = 0;
int dataCacheMissFlag_NullOps = 0;
uint64_t dataMiss_Reg3 = 0;
uint64_t dataMiss_Reg1 = 0;
uint64_t dataMiss_Reg5 = 0;


// flags for mem stage stalls
int onlyMemStageStall = 0;
int onlyMemStageStallCycles = 0;


int instructionCacheMissPropogationPreventionFlag = 0;



uint32_t *requestedData_instr_cache;
uint32_t *requestedData_data_cache;

/* result from bp_predict */
uint64_t bp_predict_result;

int b2b_miss_check;

/* global pipeline state */
CPU_State CURRENT_STATE;

Pipe_Reg_IFtoDE_struct Pipe_Reg_IFtoDE;

/* global variable for holding intermediate registers and information*/
Pipe_Reg_DEtoEX_struct Pipe_Reg_DEtoEX;

Pipe_Reg_EXtoMEM_struct Pipe_Reg_EXtoMEM;

Pipe_Reg_MEMtoWB_struct Pipe_Reg_MEMtoWB;

int stagesToStall = 0;
int numOfNullOps = 0;
//int branchFlag = 0;
int squashInstruction = 0;
int bcond_taken = 0;
int RUN_BIT;
uint32_t stat_cycles;
uint32_t stat_inst_retire;
int emptyMemStage = 0;


//////////////////////////////////////////////
// HELPER FUNCTIONS
/////////////////////////////////////////////
// helper function to insert null op
void insertNullOp(int stageNumber, int numOfCycles){
  stagesToStall = stageNumber;
  numOfNullOps = numOfCycles;
}



// Helper function for flushing
void flush_helper(uint64_t ctarget) {

CURRENT_STATE.PC = ctarget;

Pipe_Reg_IFtoDE.instructionCode = 0;

strcpy(Pipe_Reg_DEtoEX.instructionName, "NONE");


int update_result = cache_update(instructionCache, ctarget);

    // data found in instruction cache
    if (update_result == 1) {
      //Pipe_Reg_IFtoDE.PC = CURRENT_STATE.PC;
     // TODO Make no propogated data being carried over between intermediatestruts
     insertNullOp(3, 1);

    }
    // data not found in instruction cache
    else {
      instructionCacheMissFlag = 1;
      if(numOfNullOps == 0){
        numOfNullOps = 51;
      }
      insertNullOp(2,numOfNullOps);
      return;
    }



}



// Help function to update PC Counter
void setValuesFromDEtoEX_EXtoMEM(){
  Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);
  Pipe_Reg_EXtoMEM.Reg8 = Pipe_Reg_DEtoEX.Reg8;
  Pipe_Reg_EXtoMEM.Reg6 = Pipe_Reg_DEtoEX.Reg6;

  Pipe_Reg_EXtoMEM.setFlagCheck = Pipe_Reg_DEtoEX.setFlagCheck;
  Pipe_Reg_EXtoMEM.writeFlag = Pipe_Reg_DEtoEX.writeFlag;
  Pipe_Reg_EXtoMEM.readFlag = Pipe_Reg_DEtoEX.readFlag;
  Pipe_Reg_EXtoMEM.branchFlag = 0;



}
// Helper Function to ouput bits at desired index (SIGNED)
int32_t getBits_S(int x, int y) {
  return ((int32_t)Pipe_Reg_IFtoDE.instructionCode << (31-x)) >> (31-x+y);
}
// Helper Function to ouput bits at desired index (UNSIGNED)
uint32_t getBits_US(int x, int y) {
  return ((Pipe_Reg_IFtoDE.instructionCode << (31-x)) >> (31-x+y));
}
// Helper function to set flags
void setFlag(int64_t value){

  if(value == 0){
    Pipe_Reg_EXtoMEM.ZFlag = 1;
  }
  else {
    Pipe_Reg_EXtoMEM.ZFlag = 0;
  }
  if(value < 0){
    Pipe_Reg_EXtoMEM.NFlag = 1;
  }
  else {
    Pipe_Reg_EXtoMEM.NFlag = 0;
  }
}



// HELPER for control hazards and updating the PC

//////////////////////////////////////////////
// DATA HAZARD FUNCTIONS
/////////////////////////////////////////////
// helper function to check data hazards
void cycleDataHazardsCheck(){
  int inst_decrement_check = 0;
  if (strcmp(Pipe_Reg_DEtoEX.instructionName, "HLT") != 0) {
    if (Pipe_Reg_MEMtoWB.Reg8 !=31){
      // Regular Data Forwarding if operation is 2 instructions away
      if (Pipe_Reg_DEtoEX.Reg6 == Pipe_Reg_MEMtoWB.Reg8) {
        Pipe_Reg_DEtoEX.Reg1 = Pipe_Reg_MEMtoWB.Reg3;
      }
      if (Pipe_Reg_DEtoEX.Reg7 == Pipe_Reg_MEMtoWB.Reg8) {
        Pipe_Reg_DEtoEX.Reg2 = Pipe_Reg_MEMtoWB.Reg3;
      }
    }
    if (Pipe_Reg_EXtoMEM.Reg8 !=31){
      if (Pipe_Reg_DEtoEX.Reg6 == Pipe_Reg_EXtoMEM.Reg8) {
        if(Pipe_Reg_EXtoMEM.readFlag == 1){
          insertNullOp(3,1);
        }
        else {
          // Regular Data Forwarding
          Pipe_Reg_DEtoEX.Reg1 = Pipe_Reg_EXtoMEM.Reg3;
        }
      }

      if (Pipe_Reg_DEtoEX.Reg7 == Pipe_Reg_EXtoMEM.Reg8 ) {
        if(Pipe_Reg_EXtoMEM.readFlag == 1){
          // Load Use data hazard
          insertNullOp(3,1);
        }
        else {

          // Regular Data Forwarding
          Pipe_Reg_DEtoEX.Reg2 = Pipe_Reg_EXtoMEM.Reg3;

        }
      }
    }
  }
}

// Mem data hazards check
// Handles Data Dependency for a STUR after LDUR
void memDataHazardsCheck(){
  if(Pipe_Reg_EXtoMEM.readFlag == 1 && Pipe_Reg_DEtoEX.writeFlag == 1 ){
    if(Pipe_Reg_DEtoEX.Reg6 == Pipe_Reg_EXtoMEM.Reg8){
      insertNullOp(3,1);
    }
  }
}

//////////////////////////////////////////////
// EXECUTE FUNCTIONS
/////////////////////////////////////////////

// Helper function to populate Pipe_Reg_DEtoEX struct

void DEtoEX_fill(char* format, char* instructionName) {
  strcpy(Pipe_Reg_DEtoEX.instructionName, instructionName);
  Pipe_Reg_DEtoEX.PC = Pipe_Reg_IFtoDE.PC;
  Pipe_Reg_DEtoEX.b2b_missFlag = Pipe_Reg_IFtoDE.b2b_missFlag;


  if (strcmp(format, "R") == 0) {

    Pipe_Reg_DEtoEX.Reg6 = getBits_US(20,16);
    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(12,10);

    Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];
    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];

    return;
  }

  if (strcmp(format, "R2") == 0) {

    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(21,10);

    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    return;
  }

  if (strcmp(format, "CB") == 0) {


    Pipe_Reg_DEtoEX.Reg4 = getBits_S(23,5);
    Pipe_Reg_DEtoEX.predicted_PC = Pipe_Reg_IFtoDE.predicted_PC;
    Pipe_Reg_DEtoEX.Reg6 = getBits_US(4,0);
    Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];

    return;
  }

  if (strcmp(format, "COMP") == 0) {

    Pipe_Reg_DEtoEX.Reg6 = getBits_US(20,16);
    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(15,10);

    Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];
    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    return;
  }

  if ((strcmp(format, "D") == 0) || (strcmp(format, "D1") == 0)) {

    if (strcmp(format, "D1") == 0){


      Pipe_Reg_DEtoEX.Reg6 = getBits_US(4,0);

      Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];

    }

    else {
      Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);
      Pipe_Reg_DEtoEX.Reg3 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg8];

    }

    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(20,12);

    //size
    Pipe_Reg_DEtoEX.Reg5 = getBits_US(31,30);

    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    return;
  }

  if (strcmp(format, "I") == 0) {

    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(21, 16);
    Pipe_Reg_DEtoEX.Reg5 = getBits_S(15,10);

    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    return;
  }

  if (strcmp(format, "MOVZ") == 0) {

    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_US(20, 5);

    return;
  }

  if (strcmp(format, "MUL") == 0) {

    Pipe_Reg_DEtoEX.Reg6 = getBits_US(20,16);
    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];
    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    return;
  }

  if (strcmp(format, "CMP") == 0) {

    Pipe_Reg_DEtoEX.Reg7 = getBits_US(9,5);
    Pipe_Reg_DEtoEX.Reg8 = getBits_US(4,0);

    Pipe_Reg_DEtoEX.Reg4 = getBits_S(12, 10);
    Pipe_Reg_DEtoEX.Reg6 = getBits_US(20,16);

    Pipe_Reg_DEtoEX.Reg2 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg7];
    Pipe_Reg_DEtoEX.Reg1 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg6];

    return;
  }

  if (strcmp(format, "IW") == 0) {


    Pipe_Reg_DEtoEX.Reg4 = getBits_S(20, 5);
    return;
  }

  if (strcmp(format, "BR") == 0) {

    Pipe_Reg_DEtoEX.Reg2 = getBits_US(9,5);

    Pipe_Reg_DEtoEX.Reg7 = CURRENT_STATE.REGS[Pipe_Reg_DEtoEX.Reg2];

    Pipe_Reg_DEtoEX.branchFlag = 1;

    Pipe_Reg_DEtoEX.predicted_PC = Pipe_Reg_IFtoDE.predicted_PC;

    return;

  }

  if (strcmp(format, "B") == 0) {
    Pipe_Reg_DEtoEX.Reg4 = getBits_S(25, 0);
    Pipe_Reg_DEtoEX.branchFlag = 1;
    Pipe_Reg_DEtoEX.predicted_PC = Pipe_Reg_IFtoDE.predicted_PC;


    return;
  }

  if (strcmp(format, "CB") == 0) {


    Pipe_Reg_DEtoEX.Reg4 = getBits_S(23, 5);

    Pipe_Reg_DEtoEX.branchFlag = 1;

    return;
  }
}




//////////////////////////////////////////////
// EXECUTE FUNCTIONS
/////////////////////////////////////////////




void add_ER(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg1 + Pipe_Reg_DEtoEX.Reg2;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}
void sub_ER(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 - Pipe_Reg_DEtoEX.Reg1;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}
// IMMEDIATE FUNCITONS
void add_IM(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 + Pipe_Reg_DEtoEX.Reg4;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}

void sub_IM(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 - Pipe_Reg_DEtoEX.Reg4;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}


// CBNZ and CBZ FUNCTIONS

void cbnz_instruction() {

  uint64_t ctarget;

  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);
  if (Pipe_Reg_DEtoEX.Reg1 != 0) {
    // TODO: Do we still update Pc through the pc state?
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
  ctarget = Pipe_Reg_DEtoEX.PC + 4;
  bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }



}

void cbz_instruction() {

  uint64_t ctarget;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);
  if (Pipe_Reg_DEtoEX.Reg1 == 0) {

    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);
    Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;

   bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);


  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

}


  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }
}


// AND FUNCTIONS
void and_SR(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg1 & Pipe_Reg_DEtoEX.Reg2;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}


// EOR FUNCTION
void eor_SR(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg1 ^ Pipe_Reg_DEtoEX.Reg2;



}

// ORR FUNCTION
void orr_SR(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg1 | Pipe_Reg_DEtoEX.Reg2;



}

// MUL FUCNTION
void mul(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg1 * Pipe_Reg_DEtoEX.Reg2;

}

// MOVZ FUNCTION
void movz() {
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg4;

}


// CMP:ER FUNCTION
void cmp_ER(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 - Pipe_Reg_DEtoEX.Reg1;

  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {

    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}

// CMP:IM FUNCTION
void cmp_IM(){
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 & Pipe_Reg_DEtoEX.Reg4;
  if (Pipe_Reg_DEtoEX.setFlagCheck == 1) {
    setFlag(Pipe_Reg_EXtoMEM.Reg3);
  }
}

// BEQ
void beq() {

  uint64_t ctarget;


  int flag_z = CURRENT_STATE.FLAG_Z ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_z = Pipe_Reg_MEMtoWB.ZFlag ;

  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_z = Pipe_Reg_EXtoMEM.ZFlag;
  }

  if (flag_z == 1) {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);
  }
  else {
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);
  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);

  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }
}


// BNE
void bne() {

  uint64_t ctarget;

  int flag_z = CURRENT_STATE.FLAG_Z ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_z = Pipe_Reg_MEMtoWB.ZFlag ;
  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_z = Pipe_Reg_EXtoMEM.ZFlag;
  }
  if (flag_z == 0) {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);


  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);




  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }

}

// BGT
void bgt() {

  uint64_t ctarget;

  int flag_z = CURRENT_STATE.FLAG_Z ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_z = Pipe_Reg_MEMtoWB.ZFlag ;

  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_z = Pipe_Reg_EXtoMEM.ZFlag;
  }

  int flag_n = CURRENT_STATE.FLAG_N ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_n = Pipe_Reg_MEMtoWB.NFlag ;

  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_n = Pipe_Reg_EXtoMEM.NFlag;
  }

  if (flag_z == 0 && flag_n== 0)   {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);



  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }
}

// BLT
void blt() {

  uint64_t ctarget;

  int flag_n = CURRENT_STATE.FLAG_N ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_n = Pipe_Reg_MEMtoWB.NFlag ;
  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_n = Pipe_Reg_EXtoMEM.NFlag;
  }
  if (flag_n != 0) {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);


  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }
}

// BGE
void bge() {
  uint64_t ctarget;

  int flag_n = CURRENT_STATE.FLAG_N ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_n = Pipe_Reg_MEMtoWB.NFlag ;
  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_n = Pipe_Reg_EXtoMEM.NFlag;
  }
  if (flag_n == 0) {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);



  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }

}

// BLE
void ble() {

  uint64_t ctarget;

  int flag_z = CURRENT_STATE.FLAG_Z ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_z = Pipe_Reg_MEMtoWB.ZFlag ;

  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_z = Pipe_Reg_EXtoMEM.ZFlag;
  }

  int flag_n = CURRENT_STATE.FLAG_N ;
  if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
    flag_n = Pipe_Reg_MEMtoWB.NFlag ;

  }
  if(Pipe_Reg_EXtoMEM.setFlagCheck == 1){
    flag_n = Pipe_Reg_EXtoMEM.NFlag;
  }
  if (!(flag_z == 0 && flag_n == 0)) {
    ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);

    bp_update(1, 1, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  else {
    ctarget = Pipe_Reg_DEtoEX.PC + 4;
    Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;
    bp_update(1, 0, Pipe_Reg_DEtoEX.PC, ctarget);

  }
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);

  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC)) {
      flush_helper(ctarget);
  }

}

// B
void b() {

  uint64_t ctarget;


  ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg4 * 4);
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;

  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);

  bp_update(0, 1, Pipe_Reg_DEtoEX.PC, ctarget);



  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC) || (Pipe_Reg_DEtoEX.b2b_missFlag == 1)) {
      flush_helper(ctarget);
  }

  return;
}

// BR
void br() {

  uint64_t ctarget;
  ctarget = Pipe_Reg_DEtoEX.PC + (Pipe_Reg_DEtoEX.Reg2 * 4);
  Pipe_Reg_EXtoMEM.branchFlag = Pipe_Reg_DEtoEX.branchFlag;
  Pipe_Reg_EXtoMEM.PC = Pipe_Reg_DEtoEX.PC;

  strcpy(Pipe_Reg_EXtoMEM.instructionName, Pipe_Reg_DEtoEX.instructionName);

  bp_update(0, 1, Pipe_Reg_DEtoEX.PC, ctarget);


  if ((ctarget !=  Pipe_Reg_DEtoEX.predicted_PC) || (Pipe_Reg_DEtoEX.b2b_missFlag == 1)) {
      flush_helper(ctarget);
  }

  return;
}


// HLT
void hlt() {
  insertNullOp(3,3);
}

// lsl immediate

void lsl_im() {

  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 << (Pipe_Reg_DEtoEX.Reg5^ 0x3f);

}

// lsr immediate
void lsr_im() {
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 >> (Pipe_Reg_DEtoEX.Reg4);

}

// STUR
void stur(){

  //size
  Pipe_Reg_EXtoMEM.Reg5 = Pipe_Reg_DEtoEX.Reg5;


  // Location in Mem to write to
  Pipe_Reg_EXtoMEM.Reg3 = Pipe_Reg_DEtoEX.Reg2 + Pipe_Reg_DEtoEX.Reg4;

  // Value to be written
  Pipe_Reg_EXtoMEM.Reg1 = Pipe_Reg_DEtoEX.Reg1;



}







// LDUR
void ldur(){

  Pipe_Reg_EXtoMEM.Reg5 = Pipe_Reg_DEtoEX.Reg5;
  // Location in Mem to read from
  Pipe_Reg_EXtoMEM.Reg1 = Pipe_Reg_DEtoEX.Reg2 + Pipe_Reg_DEtoEX.Reg4;


}

//////////////////////////////////////////////
// MAIN FUNCTIONS FOR CYCLE
/////////////////////////////////////////////

void pipe_init()
{
  memset(&CURRENT_STATE, 0, sizeof(CPU_State));
  CURRENT_STATE.PC = 0x00400000;
  strcpy(Pipe_Reg_MEMtoWB.instructionName, "NONE");
  strcpy(Pipe_Reg_EXtoMEM.instructionName, "NONE");
  strcpy(Pipe_Reg_DEtoEX.instructionName, "NONE");

  // create instruction cache
  instructionCache = cache_new(64, 4, 32);

  // create data cache
  dataCache = cache_new(256,8,32);

}

// MAIN FUNCTION:
// HIGH LEVEL IMPLEMENTATION:
// -To handle data dependency, we preform a check at the beggining of the cycle,forwarding any necessary data to DEtoEX Pipe Reg
// -To handle control hazards, we check for nonconditional branches one cycle after the branch executes (branch is at MEM), and see if it was bcond_taken
//    where itll adjust the PC for the stall that was implemented
// - Each Stage has an if that will cause the stage not to run, such that null ops and stalls can be easily preformed
// - In order to "clear" Pipeline Registers easily, we set the register# of Rd (.R8) field to '\0', which is null, preventing the same instruction to
//    happen twice if there is no instruction behind it

void pipe_cycle()
{
  // cycle data check
  cycleDataHazardsCheck();
  pipe_stage_wb();
  pipe_stage_mem();
  pipe_stage_execute();
  pipe_stage_decode();
  pipe_stage_fetch();

  if(dataCacheMissFlag_NullOps == 1){
    instructionCacheMissPropogationPreventionFlag = 0;
    if (numOfNullOps ==  48) {
	onlyMemStageStall = 1;
        onlyMemStageStallCycles = 49;
        //instructionCacheMissPropogationPreventionFlag = 0;
    }
    else if(numOfNullOps == 0){
	numOfNullOps = 51;
    }

    insertNullOp(4,numOfNullOps);
    dataCacheMissFlag_NullOps = 0;
  }

  // checking null ops
  if(numOfNullOps > 0){
    numOfNullOps = numOfNullOps -1;
  }

  /*
  if (instructionCacheMissPropogationPreventionFlag > 0) {
        instructionCacheMissPropogationPreventionFlag = instructionCacheMissPropogationPreventionFlag + 1;
  }
  */

  if (numOfNullOps == 0 ){
    stagesToStall = 0;
    instructionCacheMissPropogationPreventionFlag = 0;
  }





}

void pipe_stage_fetch()
{





  if ((stagesToStall > 0  && numOfNullOps > 0) )  //|| (branchFlag == 2))
  {
    return;
  }
  else {
    //Pipe_Reg_IFtoDE.instructionCode = mem_read_32(CURRENT_STATE.PC);

    if (instructionCacheMissFlag == 1) {
      insert_in_Cache(instructionCache);
      instructionCacheMissFlag = 0;
      //instructionCacheMissPropogationPreventionFlag = 1;
    }

    int update_result = cache_update(instructionCache, CURRENT_STATE.PC);

    // data found in instruction cache
    if (update_result == 1) {
      Pipe_Reg_IFtoDE.PC = CURRENT_STATE.PC;

      int offsetBits = (int) getPC_bits(4,0, CURRENT_STATE.PC) / 4;
      Pipe_Reg_IFtoDE.instructionCode = (requestedData_instr_cache[offsetBits]);
    }
    // data not found in instruction cache
    else {
      instructionCacheMissFlag = 1;
      insertNullOp(2,50);
      instructionCacheMissPropogationPreventionFlag = 1;
      return;
    }



    bp_predict(Pipe_Reg_IFtoDE.PC);
    CURRENT_STATE.PC = bp_predict_result;

    Pipe_Reg_IFtoDE.predicted_PC = bp_predict_result;
  }

  if (b2b_miss_check == 1) {

	b2b_miss_check = 0;
        Pipe_Reg_IFtoDE.b2b_missFlag = 1;
  }
  else {
	Pipe_Reg_IFtoDE.b2b_missFlag = 0;
 }
}

void pipe_stage_decode()
{

  /*
  if (instructionCacheMissPropogationPreventionFlag > 2) {
	Pipe_Reg_IFtoDE.instructionCode = 0;
        //strcpy(Pipe_Reg_FEtoDE.instructionName, "NONE");
	return;
  }
  */


  if ((stagesToStall > 1 && numOfNullOps > 0) || (Pipe_Reg_IFtoDE.instructionCode == 0)){
    return;
  }
  else {
    Pipe_Reg_DEtoEX.readFlag = 0;
    Pipe_Reg_DEtoEX.writeFlag = 0;
    Pipe_Reg_DEtoEX.setFlagCheck = 0;
    Pipe_Reg_DEtoEX.Reg8 = 31;

    uint32_t opCode1;
    uint32_t opCode2;
    uint32_t leadingBit = getBits_US(31,31);
    //first bit 0
    if (leadingBit == 0) {
      opCode1 = getBits_US(31, 24);
      // contains the condition bits and the secondary opcode bit for b.cond
      opCode2 = getBits_US(3,0);
      //b.cond
      if (opCode1 == 0x54) {
        //b.beq
        if (opCode2 == 0x0) {
          DEtoEX_fill("CB", "BEQ");

          // branchFlag = 3;

          return;
        }
        //b.bne
        if (opCode2 == 0x1) {
          DEtoEX_fill("CB", "BNE");
          // branchFlag = 3;

          return;
        }
        //b.bgt
        if (opCode2 == 0xc) {
          DEtoEX_fill("CB", "BGT");
          // branchFlag = 3;

          return;
        }
        //b.blt
        if (opCode2 == 0xb) {
          DEtoEX_fill("CB", "BLT");
          // branchFlag = 3;

          return;
        }
        //b.bge
        if (opCode2 == 0xa) {
          DEtoEX_fill("CB", "BGE");
          // branchFlag = 3;

          return;
        }
        //b.ble
        if (opCode2 == 0xD) {
          DEtoEX_fill("CB", "BLE");
          // branchFlag = 3;

          return;
        }

      }
      opCode1 = getBits_US(31,21);
      opCode2 = getBits_US(11,10);
      //ldurb
      if ((opCode1 == 0x1c2) && (opCode2 == 0x0)) {
        Pipe_Reg_DEtoEX.readFlag = 1;
        DEtoEX_fill("D", "LDURB");
        return;
      }
      //ldurh
      if ((opCode1 == 0x3c2) && (opCode2 == 0x0)) {
        Pipe_Reg_DEtoEX.readFlag = 1;
        DEtoEX_fill("D", "LDURH");
        return;
      }
      //sturb
      if ((opCode1 == 0x1c0) && (opCode2 == 0x0)) {
        Pipe_Reg_DEtoEX.writeFlag = 1;
        DEtoEX_fill("D1", "STURB");
        return;
      }
      //sturh
      if ((opCode1 == 0x3c0) && (opCode2 == 0x0)) {
        Pipe_Reg_DEtoEX.writeFlag = 1;
        DEtoEX_fill("D1", "STURH");
        return;
      }
      opCode1 = getBits_US(31,26);

      //b
      if (opCode1 == 0x5) {
        DEtoEX_fill("B","B");
        // branchFlag = 3;
        return;
      }



    }
    //first bit is 1
    {
      opCode1 = getBits_US(31,21);
      opCode2 = getBits_US(11,10);
      //ldur
      if (((opCode1 == 0x7c2) || (opCode1 == 0x5c2)) && (opCode2 == 0x0)) {

        Pipe_Reg_DEtoEX.readFlag = 1;
        DEtoEX_fill("D", "LDUR");
        return;

      }
      //stur
      if (((opCode1 == 0x7c0) || (opCode1 == 0x5c0 )) && (opCode2 == 0x0)) {

        Pipe_Reg_DEtoEX.writeFlag = 1;
        DEtoEX_fill("D1", "STUR");
        return;
      }
      opCode2 = getBits_US(15,10);
      //mul
      if ((opCode1 == 0x4d8) && (opCode2 == 0x1f)) {
        DEtoEX_fill("MUL", "MUL");
        return;
      }
      opCode2 = getBits_US(4,0);

      //hlt
      if ((opCode1 == 0x6a2) && (opCode2 == 0x0)) {
        DEtoEX_fill("IW", "HLT");
        return;
      }
      //cmp (ER)
      if (((opCode1 == 0x759) || (opCode1 == 0x758)) && (opCode2 == 0x1f)) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("CMP", "CMP:ER");
        return;
      }
      //add (ER)
      if ((opCode1 == 0x459) || (opCode1 == 0x458)) {
        DEtoEX_fill("R", "ADD:ER");
        return;
      }
      //adds (ER)
      if ((opCode1 == 0x559) || (opCode1 == 0x558)) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("R", "ADDS:ER");
      }
      //sub (ER)
      if ((opCode1 == 0x659) || (opCode1 == 0x658)) {
        DEtoEX_fill("R", "SUB:ER");
        return;
      }
      //subs (ER)
      if ((opCode1 == 0x759) || (opCode1 == 0x758)) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("R", "SUBS:ER");
      }

      opCode1 = getBits_US(31,24);
      opCode2 = getBits_US(21,20);
      //and (SR)
      if ((opCode1 == 0x8a) && (opCode2 == 0x0)) {
        DEtoEX_fill("COMP", "AND:SR");
        return;
      }
      //ands (SR)
      if ((opCode1 == 0xea) && (opCode2 == 0x0)) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("COMP", "ANDS:SR");
      }
      //eor (SR)
      if ((opCode1 == 0xca) && (opCode2 == 0x0)) {
        DEtoEX_fill("COMP", "EOR:SR");
        return;
      }
      //orr
      if ((opCode1 == 0xaa) && (opCode2 == 0x0)) {
        DEtoEX_fill("COMP", "ORR:SR");
        return;
      }
      opCode2 = getBits_US(4,0);
      //cmp (IM)
      if (opCode1 == 0xf1 && (opCode2 == 0x1f)) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("R2", "CMP:IM");
        return;
      }
      //add (IM)
      if (opCode1 == 0x91) {
        DEtoEX_fill("R2", "ADD:IM");
        return;
      }

      //adds (IM)
      if (opCode1 == 0xb1) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("R2", "ADDS:IM");
        return;
      }
      //cbnz
      if (opCode1 == 0xb5) {
        DEtoEX_fill("CB", "CBNZ");
        // branchFlag = 3;

        return;
      }
      //cbz
      if (opCode1 == 0xb4) {
        DEtoEX_fill("CB", "CBZ");
        // branchFlag = 3;

        return;
      }
      //sub (IM)
      if (opCode1 == 0xd1) {
        DEtoEX_fill("R2", "SUB:IM");
        return;

      }
      //subs (IM)
      if (opCode1 == 0xf1) {
        Pipe_Reg_DEtoEX.setFlagCheck = 1;
        DEtoEX_fill("R2", "SUBS:IM");
        return;
      }
      opCode1 = getBits_US(31,22);
      opCode2 = getBits_US(15,10);

      //lsl (IM)
      if ((opCode1 == 0x34d) && (opCode2 != 0x3F)) {
        DEtoEX_fill("I", "LSL:IM");
        return;
      }

      //lsr (IM)
      if ((opCode1 == 0x34d) && (opCode2 == 0x3f)) {
        DEtoEX_fill("I", "LSR:IM");
        return;
      }

      opCode1 = getBits_US(31,23);

      //moz
      if (opCode1 == 0x1a5) {
        DEtoEX_fill("MOVZ", "MOVZ");
        return;
      }

      opCode1 = getBits_US(31,10);
      opCode2 = getBits_US(4,0);

      //br
      if ((opCode1 == 0x3587c0) && (opCode2 == 0x0)) {
        DEtoEX_fill("BR", "BR");
        // branchFlag = 3;

        return;
      }
    }


  }

}



void pipe_stage_execute()
{
  /*
  if (instructionCacheMissPropogationPreventionFlag > 3) {
        strcpy(Pipe_Reg_DEtoEX.instructionName, "NONE");
       return;
  }
  */

  if ((stagesToStall > 2  && numOfNullOps > 0)){
    return;
  }
  else {
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ADD:ER") == 0){
      add_ER();
      setValuesFromDEtoEX_EXtoMEM();
      return;
    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ADDS:ER") == 0){
      add_ER();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "SUB:ER") == 0){
      sub_ER();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "SUBS:ER") == 0){
      sub_ER();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }

    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ADD:IM") == 0){
      add_IM();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ADDS:IM")== 0){
      //
      add_IM();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "SUB:IM")== 0){

      sub_IM();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "SUBS:IM")== 0){


      sub_IM();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "CBNZ") == 0){

      cbnz_instruction();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "CBZ") == 0){

      cbz_instruction();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "AND:SR") == 0){


      and_SR();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ANDS:SR") == 0){


      and_SR();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "EOR:SR") == 0){

      eor_SR();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "ORR:SR") == 0){

      orr_SR();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "LDUR") == 0){
      ldur();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "LDURB") == 0){
      ldur();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "LDURH") == 0){
      ldur();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "STUR") == 0){
      stur();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "STURB") == 0){
      stur();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "STURH") == 0){
      stur();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "LSL:IM") == 0){
      lsl_im();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "LSR:IM") == 0){
      lsr_im();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "MOVZ") == 0){

      movz();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "MUL") == 0){

      mul();
      setValuesFromDEtoEX_EXtoMEM();
      return;


    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "CMP:ER") == 0){

      cmp_ER();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "CMP:IM") == 0){

      cmp_IM();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "HLT") == 0){
      hlt();
      setValuesFromDEtoEX_EXtoMEM();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BR") == 0){
      br();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "B") == 0){
      b();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BEQ") == 0){
      beq();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BNE") == 0){
      bne();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BGT") == 0){
      bgt();
      return;
    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BLT") == 0){
      blt();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BGE") == 0){
      bge();
      return;

    }
    if(strcmp(Pipe_Reg_DEtoEX.instructionName, "BLE") == 0){
      ble();
      return;


    }
    if (strcmp(Pipe_Reg_DEtoEX.instructionName, "NONE") == 0){
	setValuesFromDEtoEX_EXtoMEM();
	return;
    }

  }

}

void pipe_stage_mem()
{

  
  if ((onlyMemStageStall == 1) && (onlyMemStageStallCycles > 0)) {

	onlyMemStageStallCycles = onlyMemStageStallCycles - 1;
	if (onlyMemStageStallCycles == 0) {
		onlyMemStageStall = 0;
	}
  }
 
  /* 
  else if (instructionCacheMissPropogationPreventionFlag > 4){
	//strcpy(Pipe_Reg_EXtoMEM.instructionName, "NONE");

	return;
  }
  */

  else if (stagesToStall > 3 && numOfNullOps > 0){
    return;
  }



  //else if (stagesToStall > 3 && numOfNullOps > 0){
  //  return;
  //}

  else {
    //mem data hazrds Checks
    memDataHazardsCheck();

    // carry field PC Counter
    Pipe_Reg_MEMtoWB.PC = Pipe_Reg_EXtoMEM.PC;

    // carry field register number RD
    Pipe_Reg_MEMtoWB.Reg8 = Pipe_Reg_EXtoMEM.Reg8;

    // carry field register number RM
    // Pipe_Reg_MEMtoWB.Reg6 = Pipe_Reg_EXtoMEM.Reg6;

    // carry over flags z and n
    Pipe_Reg_MEMtoWB.setFlagCheck = Pipe_Reg_EXtoMEM.setFlagCheck;
    Pipe_Reg_MEMtoWB.NFlag = Pipe_Reg_EXtoMEM.NFlag;
    Pipe_Reg_MEMtoWB.ZFlag = Pipe_Reg_EXtoMEM.ZFlag;



    // carry field Result
    Pipe_Reg_MEMtoWB.Reg3 = Pipe_Reg_EXtoMEM.Reg3;

    // carry over read flag
    Pipe_Reg_MEMtoWB.readFlag = Pipe_Reg_EXtoMEM.readFlag;

    // carry over write flags
    Pipe_Reg_MEMtoWB.writeFlag = Pipe_Reg_EXtoMEM.writeFlag;

    Pipe_Reg_MEMtoWB.branchFlag = Pipe_Reg_EXtoMEM.branchFlag;

    // carry instruction name
    strcpy(Pipe_Reg_MEMtoWB.instructionName, Pipe_Reg_EXtoMEM.instructionName);

    // if memory needs to be written
    if (Pipe_Reg_EXtoMEM.writeFlag == 1) {

      if (dataCacheMissFlag == 1) {
      	insert_in_Cache(dataCache);
      	dataCacheMissFlag = 0;
        int update_result = cache_update(dataCache, dataMiss_Reg3);
        if (update_result == 1) {
  	        modify_in_cache(dataCache, dataMiss_Reg1, dataMiss_Reg5);
        }
      }
      else {
        int update_result = cache_update(dataCache, Pipe_Reg_EXtoMEM.Reg3);
        if (update_result == 1) {
  	        modify_in_cache(dataCache, Pipe_Reg_EXtoMEM.Reg1, Pipe_Reg_EXtoMEM.Reg5);
        }
        else {
  	        dataCacheMissFlag = 1;
            dataCacheMissFlag_NullOps = 1;
            dataMiss_Reg3 = Pipe_Reg_EXtoMEM.Reg3;
            dataMiss_Reg1 = Pipe_Reg_EXtoMEM.Reg1;
            dataMiss_Reg5 = Pipe_Reg_EXtoMEM.Reg5;
        }
      }
    }

    // if memory needs to be read
    // holding memory read result in Pipe_Reg_MEMtoWB.Reg3
    if (Pipe_Reg_EXtoMEM.readFlag == 1) {
       if (dataCacheMissFlag == 1) {
		       insert_in_Cache(dataCache);
	         dataCacheMissFlag = 0;
	       }

       int update_result = cache_update(dataCache, Pipe_Reg_EXtoMEM.Reg1);

       // data found in instruction cache
       if (update_result == 1) {
	        int offsetBits = (int) getPC_bits(4,0, Pipe_Reg_EXtoMEM.Reg1) / 4;
        	if (!(Pipe_Reg_EXtoMEM.Reg5) == 3) {
        		int shift_size = (int) (8* (int) pow(2,Pipe_Reg_EXtoMEM.Reg5));
        		Pipe_Reg_MEMtoWB.Reg3 = (requestedData_data_cache[offsetBits] << shift_size) >> shift_size;
          }
        	else {
     	       uint64_t read_end = (requestedData_data_cache[offsetBits]);
             uint64_t read_beg = (requestedData_data_cache[offsetBits + 1]);
     	       Pipe_Reg_MEMtoWB.Reg3 = (read_beg << 32) + read_end;
         	 }
       // TODO Make no propogated data being carried over between intermediatestruts
       }
       // data not found in instruction cache
       else {
         dataCacheMissFlag = 1;
         dataCacheMissFlag_NullOps = 1;
       }
    }
  }

  if (stagesToStall == 3){
    Pipe_Reg_EXtoMEM.Reg8 = 32;
  }

}

// references struct Pipe_Reg_MEMtoWB
void pipe_stage_wb()
{

/*
  if (instructionCacheMissPropogationPreventionFlag > 4) {
	strcpy(Pipe_Reg_MEMtoWB.instructionName, "NONE");
        return;
  }
*/
   //if ((stagesToStall > 4  && numOfNullOps > 0) || (dataCacheMissFlag == 1)){
  if ((stagesToStall > 4  && numOfNullOps > 0) || (dataCacheMissFlag == 1)){
    return;
  }

  else {
	
    if (((32 > Pipe_Reg_MEMtoWB.Reg8) ) && (strcmp(Pipe_Reg_MEMtoWB.instructionName, "NONE") != 0))
     {
      stat_inst_retire = stat_inst_retire + 1;
    }

    if(Pipe_Reg_MEMtoWB.setFlagCheck == 1){
      CURRENT_STATE.FLAG_N = Pipe_Reg_MEMtoWB.NFlag;
      CURRENT_STATE.FLAG_Z = Pipe_Reg_MEMtoWB.ZFlag;
    }
    if (strcmp(Pipe_Reg_MEMtoWB.instructionName, "HLT") == 0) {
      RUN_BIT = 0;
      cache_destroy(instructionCache);
      cache_destroy(dataCache);
    }

    if ((Pipe_Reg_MEMtoWB.writeFlag != 1) && (Pipe_Reg_MEMtoWB.branchFlag != 1) && ((31 > Pipe_Reg_MEMtoWB.Reg8))) {
      CURRENT_STATE.REGS[Pipe_Reg_MEMtoWB.Reg8] = Pipe_Reg_MEMtoWB.Reg3;
    }
  }
  Pipe_Reg_MEMtoWB.Reg8 = 35;
  strcpy(Pipe_Reg_MEMtoWB.instructionName, "NONE");
}
