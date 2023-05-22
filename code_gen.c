#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cassert>

#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>

#include "code_gen.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#define prt(x) if(x) { printf("%s\n", x); }

int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *list_index, std::unordered_map<LLVMValueRef, int[2]> *live_range);

int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef, int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, int registers_available);

/*
	Helper function to import LLVMModel
		from a .s file

*/
LLVMModuleRef importLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
	}

	return m;
}


/*
 * By: Luke Wilbur
 *
 * Debugging function used to print a unordered_map mapping
 * from instructions to an integer pair representing its 
 * (firstTimeUsed, lastTimeUsed).
 *
 * NB: Appends an extra '|' after the last item in the list
 */
void print_umap(std::unordered_map<LLVMValueRef, int[2]>* umap) {
    for (auto itr : *umap) {	
       	LLVMDumpValue(itr.first);
       	printf(" -> (%d, %d) \n ", itr.second[0], itr.second[1]); 
	}
} 



/*
	Finds liveness of each variable in a basic block

	Takes in a basic block and creates (but is provided a memory-allocated):
		list_index - a mapping from an instruction to their index in the block (an integer starting at 0)
		live_range - mapping from an instruction to a pair of integers
*/
int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *list_index, std::unordered_map<LLVMValueRef, int[2]> *live_range) {

	int index = 0;
	

	// for each instruction
	for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {		
		// if it's alloc, skip this iteration
		if (LLVMGetInstructionOpcode(instr) == LLVMAlloca) {
			continue;
		}


		(*live_range)[instr][0] = index;
		(*live_range)[instr][1] = index;	
	
		int num_operands = LLVMGetNumOperands(instr);

		// for each operand:
		for (int i = 0; i < num_operands; i++) {
			
			LLVMValueRef target = LLVMGetOperand(instr,i);
			

			if (LLVMGetInstructionOpcode(target) == LLVMAlloca) {continue;}	
		//	LLVMDumpValue(target);
			

			// if no entry exists
			if (live_range->find(target) == live_range->end()) {
				(*live_range)[target][0] = index;
				(*live_range)[target][1] = index;
			}	
			else {		// otherwise an entry exists, so we just update end		
				// update liveness
				(*live_range)[target][1] = index;
			}
		}
			
		// map the life span and head out	
		index++;
	}

	// get rid of useless instructions
	std::unordered_map<LLVMValueRef, int[2]>::iterator itr;

	for (itr = live_range->begin(); itr != live_range->end(); ) {	
		if (itr->second[1] - itr->second[0] <= 0) {
        	itr = live_range->erase(itr);
    	} else ++itr;
	}


	return 0;
}

/*
		spill 

		look at the ones being used rn
		and the one that's spilling
	
		pick one of them to be spilled (least uses)
				set that to -1, [may have to swappy?]

*/



int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, int registers_available) {	

	int index;

	std::set<int>* active;
	for (int i = 0; i < registers_available; i++) {
		active->insert(i);
	}

	for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
		// if it's alloc, skip this iteration
		if (LLVMGetInstructionOpcode(instr) == LLVMAlloca) {
			continue;
		}
		
		// if we have a start point on this instruction (likely do)

			// expire old intervals

		
			// if no registers available (size ==0)
				// spill (and pass in the index and the interval{instr and live_range})

			// else
				// assign one of the registers
				// add to active set			
		
	}	

	return 0;
}


int code_gen(char* filename) {
	assert(filename != NULL);

	LLVMModuleRef module;
	
	int registers_available = 3;
	
	module = importLLVMModel(filename);	
	
	if (module == NULL) {
		fprintf(stderr,"module is NULL!\n");
		return -1;
	}


	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_list_index = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> *block_to_live_range = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_reg_map = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	
	// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
		// COMPUTE LIVENESS

			// initialize two mappings for the given block
			std::unordered_map<LLVMValueRef,int> *list_index = new std::unordered_map<LLVMValueRef,int> ();
			std::unordered_map<LLVMValueRef, int[2]> *live_range = new std::unordered_map<LLVMValueRef, int[2]> ();
			
			// update live_range for this block
			compute_liveness(basicBlock,list_index,live_range);
			
			// assign mapping for block-level
			(*block_to_live_range)[basicBlock] = live_range;
			
			print_umap(live_range);
		// ACTUAL REGISTER ALLOCATION
	
			// reg_map, a mapping from each instruction to each physical register
			std::unordered_map<LLVMValueRef, int> *reg_map = new std::unordered_map<LLVMValueRef,int> ();
			
			// get register allocations for this block
			register_allocate(basicBlock,live_range,reg_map,registers_available);

			(*block_to_reg_map)[basicBlock] = reg_map;
		}
	}

	return 0;

}
