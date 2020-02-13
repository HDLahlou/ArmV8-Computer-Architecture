/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>


void *requestedData;

int cacheIndexToOverwrite;

uint64_t addr;




typedef struct
{

  // 0 for instruction cache
  // 1 for data cache
  int cacheTypeFlag;

  // tag bits
  uint64_t tagBits;

  // age bit will be represented by cycle number
  uint32_t ageBitFlag;

  int validBitFlag;
  int dirtyBitFlag;

  // number of ways
  int ways;

  // number of sets
  int sets;

  // pointer to array of blocks that hold data
  //void **blocksMemory;

  // pointer to array of blocks that hold data
  uint32_t *blocksMemory;

  // blocksize
  int blocksize;


} cache_t;







cache_t *cache_new(int sets, int ways, int block);
void cache_destroy(cache_t *c);
int cache_update(cache_t *c, uint64_t addr);
void insert_in_Cache(cache_t *c);
void modify_in_cache(cache_t *c, uint64_t modified_data, int stur_size);


extern uint32_t *requestedData_instr_cache;
extern uint32_t *requestedData_data_cache;


#endif
