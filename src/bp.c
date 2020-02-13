/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 * Gushu Li and Reza Jokar
 */

#include "bp.h"
#include "pipe.h"
#include <stdlib.h>
#include <stdio.h>


uint64_t bp_predict_result;
bp_t bp_t_object;

int b2b_miss_check;

// HELPER FUNCTIONS


uint64_t getPC_bits(int x, int y, uint64_t PC) {
  return ((PC << (63-x)) >> (63-x+y));
}


void PHT_helper(uint64_t PC, int takenFlag) {

	uint8_t ghr = bp_t_object.ghr;
	uint8_t pc_2xr = getPC_bits(9,2, PC);
	uint8_t xor_result = ghr ^ pc_2xr;

	int satCounter_at_index = bp_t_object.PHT[xor_result];

	if (satCounter_at_index == '\0') {
		satCounter_at_index = 0;
	}


	if (takenFlag == 0) {

		if (satCounter_at_index != 0) {
			satCounter_at_index = satCounter_at_index - 1;
		}
	}

	 if (takenFlag == 1) {

                if (satCounter_at_index != 3) {
                        satCounter_at_index = satCounter_at_index + 1;
                }
        }

	bp_t_object.PHT[xor_result] = satCounter_at_index;
	ghr = ghr << 1;
        bp_t_object.ghr = ghr + takenFlag;
}

void B2B_helper(uint64_t PC, int cond_bit, uint64_t PC_target) {

	uint8_t pc_2_index = getPC_bits(11,2, PC);

	bp_t_object.B2B[pc_2_index].valid_bit = 1;

	bp_t_object.B2B[pc_2_index].cond_bit = cond_bit;

	bp_t_object.B2B[pc_2_index].PC_target = PC_target;

	bp_t_object.B2B[pc_2_index].PC = PC;

}

int PHTLookup(uint64_t PC) {


	uint8_t ghr = bp_t_object.ghr;
        uint8_t pc_2xr = getPC_bits(9,2, PC);
        uint8_t xor_result = ghr ^ pc_2xr;

        int satCounter_at_index = bp_t_object.PHT[xor_result];

	return satCounter_at_index;


}



void bp_predict(uint64_t PC)
{
    /* Predict next PC */


    uint32_t B2B_index = getPC_bits(11,2, PC);

    if (bp_t_object.B2B[B2B_index].valid_bit == 1) {

	if (bp_t_object.B2B[B2B_index].PC == PC) {

		if (bp_t_object.B2B[B2B_index].cond_bit == 1) {

			if (PHTLookup(PC) < 2) {

				bp_predict_result = PC + 4;
				return;
			}
			bp_predict_result = bp_t_object.B2B[B2B_index].PC_target;
			return;

		}
		bp_predict_result = bp_t_object.B2B[B2B_index].PC_target;
		return;

	}

    }
    if (bp_t_object.B2B[B2B_index].valid_bit != 1) {
	b2b_miss_check = 1;
    }
    bp_predict_result = PC + 4;
    return;


}

// TODO CHECK WHAT IS MEANT BY CURRENT PC TO USE IN EXECUTRE HELPER FUNCTIONS
void bp_update(int condFlag, int takenFlag, uint64_t PC, uint64_t target)
{
    B2B_helper(PC, condFlag, target);


    if (condFlag == 1) {

        PHT_helper(PC, takenFlag); //also update GHR

    }

    /* Update BTB */


    /* Update gshare directional predictor */


    /* Update global history register */

}
