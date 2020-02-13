/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include "bp.h"
#include "shell.h"
#include <stdlib.h>
#include <stdio.h>


uint32_t *requestedData_instr_cache;
int cacheIndexToOverwrite_instr_cache;
uint64_t addr_for_overwriting_instr_cache;


uint32_t *requestedData_data_cache;
int cacheIndexToOverwrite_data_cache;
uint64_t addr_for_overwriting_data_cache;

uint32_t stat_cycles;

// insert data in cache from memory

void insert_in_Cache(cache_t *c) {

  uint32_t mem_to_insert;

  int cacheIndexToOverwrite;
  uint64_t addr_for_overwriting;

  uint64_t tagBits;

  uint64_t addr_for_wb;
  int wbFlag = 0;
  // instruction cache
  if (c[0].cacheTypeFlag == 0) {
      cacheIndexToOverwrite = cacheIndexToOverwrite_instr_cache;
      addr_for_overwriting = getPC_bits(63,5, addr_for_overwriting_instr_cache) << 5;
      tagBits = getPC_bits(63,10, addr_for_overwriting_instr_cache);

      addr_for_wb = (c[cacheIndexToOverwrite].tagBits << 10) + (cacheIndexToOverwrite << 5);
  }

  // data cache
  else {
    // printf("---------------INDEX TO MATCH: %d\n", cacheIndexToOverwrite_data_cache);

    cacheIndexToOverwrite = cacheIndexToOverwrite_data_cache;
    addr_for_overwriting = getPC_bits(63,5, addr_for_overwriting_data_cache) << 5;
    tagBits = getPC_bits(63,12, addr_for_overwriting_data_cache);

    addr_for_wb = (c[cacheIndexToOverwrite].tagBits << 12) + (cacheIndexToOverwrite << 5);


  }

  c[cacheIndexToOverwrite].tagBits = tagBits;
  c[cacheIndexToOverwrite].ageBitFlag = stat_cycles;
  // printf("WTF:%d\n",cacheIndexToOverwrite );
  c[cacheIndexToOverwrite].validBitFlag = 1;

  if (c[cacheIndexToOverwrite].dirtyBitFlag == 1) {
	wbFlag = 1;
        c[cacheIndexToOverwrite].dirtyBitFlag = 0;
  }



  // TODO need to updat metadata
  int cache_block_size = c[0].blocksize;
  // printf("****************************************8\n");

  for (int i = 0; i < 8; i++) {

    if (wbFlag == 1) {
	mem_write_32(addr_for_wb + (4 * i), (c[cacheIndexToOverwrite].blocksMemory[(i)]));
    }
    mem_to_insert = mem_read_32(addr_for_overwriting + (4 * i));
    // printf("Mem to insert: %x\n", mem_to_insert);
    c[cacheIndexToOverwrite].blocksMemory[i] = (mem_to_insert);
  }

}


void modify_in_cache(cache_t *c, uint64_t modified_data, int stur_size){

  //printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
  //printf("modified data: %x\n", modified_data);

  uint32_t unmodified_block = (c[cacheIndexToOverwrite_data_cache].blocksMemory[getPC_bits(4,0,addr_for_overwriting_data_cache) / 4]);

  //printf("unmodified block: %x\n", unmodified_block);
  uint32_t final_modified_block;

  //printf("stur size: %d\n", stur_size);
  // 8
  if (stur_size == 0) {
    final_modified_block = ((unmodified_block >> 8) << 8) + ((modified_data << 56) >> 56);
    c[cacheIndexToOverwrite_data_cache].blocksMemory[getPC_bits(4,0,addr_for_overwriting_data_cache) / 4] = (final_modified_block);
  }
  // 16
  if (stur_size == 1) {
    final_modified_block = ((unmodified_block >> 16) << 16) + ((modified_data << 48) >> 48);
    c[cacheIndexToOverwrite_data_cache].blocksMemory[getPC_bits(4,0,addr_for_overwriting_data_cache) / 4] = (final_modified_block);

  }
  // 32
  if (stur_size == 2) {
      final_modified_block = modified_data;
      c[cacheIndexToOverwrite_data_cache].blocksMemory[getPC_bits(4,0,addr_for_overwriting_data_cache) / 4] = (final_modified_block);

  }
  if (stur_size == 3) {
      uint32_t test = modified_data;
      final_modified_block = modified_data;
      c[cacheIndexToOverwrite_data_cache].blocksMemory[getPC_bits(4,0,addr_for_overwriting_data_cache) / 4] = (final_modified_block);
      //printf("test: %x\n", test);
      final_modified_block = modified_data >> 32;
      c[cacheIndexToOverwrite_data_cache].blocksMemory[((getPC_bits(4,0,addr_for_overwriting_data_cache) / 4) + 1) ] = (final_modified_block);


  }

  //printf("final modified block: %x\n",final_modified_block);

  c[cacheIndexToOverwrite_data_cache].ageBitFlag = stat_cycles;
  c[cacheIndexToOverwrite_data_cache].dirtyBitFlag = 1;

}


// instruction cache_t
  // 6 bits for set index => 2^6 => 64 indices
  // 2 bits for tag => 4 blocks

cache_t *cache_new(int sets, int ways, int block)
{

int blocksize = block;

cache_t *newcache;
newcache = malloc(sets * ways * sizeof(cache_t));

int cacheTypeFlag;
// instruction cache
if (ways == 4) {cacheTypeFlag = 0;}
// data cache
if (ways == 8) {cacheTypeFlag = 1;}
// printf("CTF: %d\n", cacheTypeFlag );

for (int i = 0; i < sets*ways; i++) {

  newcache[i].cacheTypeFlag = cacheTypeFlag;
  newcache[i].ageBitFlag = 0;
  newcache[i].validBitFlag = 0;
  newcache[i].ageBitFlag = 0;
  newcache[i].ways = ways;
  newcache[i].sets = sets;
  newcache[i].blocksMemory = malloc(4*block);
  newcache[i].blocksize = block;

}


return newcache;

}

void cache_destroy(cache_t *c)
{
  for (int i = 0; i < c[0].sets*c[0].ways; i++) {
    free(c[i].blocksMemory);
  }
  free(c);
}



int cache_update(cache_t *c, uint64_t addr)
{


uint64_t setBits;
uint64_t tagBits;
uint64_t offsetBits;


// printf("addr: %x\n", addr);
// instruction cache
if (c[0].cacheTypeFlag == 0) {

	// printf("instruction cache hit\n");
  //get set bits from addr
  setBits = getPC_bits(10,5, addr);

  // get tag bits from addr
  tagBits = getPC_bits(63,10, addr);

  // get offset bits from addr
  offsetBits = getPC_bits(4,0, addr);


}

else {
	 // printf("data cache hit\n");
  //get set bits from addr
  setBits = getPC_bits(12,5, addr);

  // get tag bits from addr
  tagBits = getPC_bits(63,12, addr);

  // get offset bits from addr
  offsetBits = getPC_bits(4,0, addr);

  // printf("SetBits: %x\n", setBits);
  // printf("TagBits: %x\n", tagBits);
}




cache_t set_to_find = c[setBits * c[0].ways];

// flag to see if data found
int isFound = 0;
int i;
int lru = INT_MAX;
int lru_ind;


for (i = setBits * c[0].ways; i < (setBits * c[0].ways) + c[0].ways; i++) {

  // printf("i: %d\n", i);
  // printf("cache tag bits: %x\n", c[i].tagBits);
  // printf("tag bits to match: %x\n", tagBits);

  if (c[i].validBitFlag == 1) {

    if (c[i].ageBitFlag < lru) {
      lru = c[i].ageBitFlag;
      lru_ind = i;
    }

    if (c[i].tagBits == tagBits) {
      isFound = 1;
      // printf("tag found\n");
      break;
    }
  }
  else {
    // printf("not valid\n");

    break;
  }

}

// found data
if (isFound == 1) {

  if (c[0].cacheTypeFlag == 0) {

    requestedData_instr_cache = c[i].blocksMemory;
    c[i].ageBitFlag = stat_cycles;


   cacheIndexToOverwrite_instr_cache = i;
   addr_for_overwriting_instr_cache = addr;


  }
  // data cache
  else {
    requestedData_data_cache = c[i].blocksMemory;
    c[i].ageBitFlag = stat_cycles;

    cacheIndexToOverwrite_data_cache = i;
    addr_for_overwriting_data_cache = addr;
  }

  return 1;
}

// data not found and entire set full
else if (i == (((setBits * c[0].ways) + c[0].ways) - 1) && (isFound == 0)) {
  // printf("++FULL CACHE++");
  // instruction cache
  if (c[0].cacheTypeFlag == 0) {

    cacheIndexToOverwrite_instr_cache = lru_ind;
    addr_for_overwriting_instr_cache = addr;

  }

  // data cache
  else {
    cacheIndexToOverwrite_data_cache = lru_ind;
    addr_for_overwriting_data_cache = addr;
  }




  // // kick out lru
  // if (c[lru_ind].dirtyBitFlag == 1) {
  //   int cache_block_size = c[0].blocksize;
  //   for (int i = 0; i < 8; i++) {
  //     mem_write_32(addr + (i*4), (c[lru_ind].blocksMemory[i]));
  //   }
  // }

  return 0;

}

// data not found but open spot
else {
  // printf("++MISS: OPEN SPOT++");

  // instruction cache
  if (c[0].cacheTypeFlag == 0) {

    cacheIndexToOverwrite_instr_cache = i;
    addr_for_overwriting_instr_cache = addr;

  }

  // data cache
  else {
    cacheIndexToOverwrite_data_cache = i;
    // printf("----------------CacheDataIndex: %d \n", cacheIndexToOverwrite_data_cache);
    addr_for_overwriting_data_cache = addr;
  }

  // put data in c[i]

  return 0;
}






}
