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

int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *num_uses, std::unordered_map<LLVMValueRef, int[2]> *live_range);

void expire_old_intervals(int index, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<int,LLVMValueRef> *active, std::set<int> *free);

int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef, int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses, int registers_available);

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
		live_range - mapping from an instruction to a pair of integers
		num_uses   - number of references to a given value
*/
int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *num_uses, std::unordered_map<LLVMValueRef, int[2]> *live_range) {

	int index = 0;
	

	// for each instruction
	for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {		
		// if it's alloc, skip this iteration
		if (LLVMGetInstructionOpcode(instr) == LLVMAlloca) {
			continue;
		}

		
		(*live_range)[instr][0] = index;
		(*live_range)[instr][1] = index;	
		(*num_uses)[instr] = 1;	

		int num_operands = LLVMGetNumOperands(instr);

		// for each operand:
		for (int i = 0; i < num_operands; i++) {
			
			LLVMValueRef target = LLVMGetOperand(instr,i);
			
			if (LLVMGetInstructionOpcode(target) == LLVMAlloca) {continue;}	

			// if no entry exists
			if (live_range->find(target) == live_range->end()) {
			
				// set start and stop to the current index
				(*live_range)[target][0] = index;
				(*live_range)[target][1] = index;

				// initialize number of uses to one
				(*num_uses)[target] = 1;
			}	
			else {		// otherwise an entry exists, so we just update end		
				// update liveness
				(*live_range)[target][1] = index;

				(*num_uses)[target]++;
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

void expire_old_intervals(int index, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<int,LLVMValueRef> *active, std::set<int> *free) {
	
	int reg;

	// expire old intervals, by looping through each active interval
		std::unordered_map<int,LLVMValueRef>::iterator itr;

		// see if any intervals have expired
		// so for each active interval
		for (itr = active->begin(); itr != active->end(); )	{
			
			printf("Expiring intervals!\n");

			// if we are at or past the interval, free the register
			if ((*live_range)[itr->second][1] <= index) {
	
// NOTE: IF YOU CAN EXPIRE AND USE THE SAME REGISTER ON A GIVEN LINE
// AND YOU NEED TO CHECK CONDITIONS (like if it's the first operand
// in the add or smt), THE CHECK WILL BE HERE. USE instr and active_itr.
		// but make sure you dont break the iterator, we're manually
		// incrementing
				printf("Index %d: expiring register %d because it expired at %d\n",index,itr->first,(*live_range)[itr->second][1]);

				// swap out registers 
				reg = itr->first;              // store register in tmp integer
				itr = active->erase(itr);	   // erase from active
				free->insert(reg);			   // add it to free registers set
		

			} else ++itr;
		}
}

int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses, int registers_available) {	
	int index = 0;

	int reg;
	
	// set of free registers
	std::set<int> *free = new std::set<int>();
	// map from register int and instruction that it holds for
	std::unordered_map<int,LLVMValueRef> *active = new std::unordered_map<int,LLVMValueRef>();	

	// set of instructions that are currently spilled
	std::set<LLVMValueRef> *spilled = new std::set<LLVMValueRef>();


	// initialize all registers to be free
	for (int i = 0; i < registers_available; i++) {
		free->insert(i);
	}

	for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
		

		// if it's alloc, skip this iteration
		if (LLVMGetInstructionOpcode(instr) == LLVMAlloca) {
			continue;
		}
		

		// expire old intervals
		expire_old_intervals(index, live_range, active, free);	

		

		// if we have a start point on this instruction (if there's an interval)
		if (live_range->find(instr) != live_range->end()) {
			
			printf("A value does spawn!\n");

			// if there's a free register
			if (free->size() > 0) {

				// get a free register	
				reg = *(free->begin());
	
				printf("Using register %d for instr: ",reg); fflush(stdout);
				LLVMDumpValue(instr); fflush(stdout);
				printf("\n");	
						
				// update free and active
				free->erase(reg);
				(*active)[reg] = instr;
				
				(*reg_map)[instr] = reg;	

			}
			// if no registers available
			else {
				printf("No registers available, we shall spill\n");

				// pick which one to spill
				
				LLVMValueRef least_used = instr;
				int least_uses = (*num_uses)[instr];				
	
				// go through all used instructions right now, and
				// get the one with the least memory look ups
				std::unordered_map<int,LLVMValueRef>::iterator itr;

				for (itr = active->begin(); itr != active->end(); ++itr)	{
					
					// if it is used less		
					if ( (*num_uses)[itr->second] < least_uses ) {
			
	
						printf("New least used, with only %d uses: \t",least_uses); fflush(stdout);
						LLVMDumpValue(itr->second); fflush(stdout);
						printf("\n"); fflush(stdout);
		
						
						// update variables
						least_used = itr->second;
						least_uses = (*num_uses)[itr->second];					
					}			

				}
				
				// get the one with the least memory lookups
			
				// then with this target:
		
				// if the least used is instr
				if (least_used == instr) { 
					
					// easy, we assign that to spill
					(*reg_map)[instr] = -1;
					spilled->insert(instr);
				}			
				// otherwise we must swap whatever target's registers are with
				else {
					
					printf("SWAPPING \t"); 
					LLVMDumpValue(least_used); fflush(stdout);
					printf("\t TO BE SPILLED INSTEAD OF");
					LLVMDumpValue(instr); fflush(stdout);
					printf("\n");
				
					// update replacement
					reg = (*reg_map)[least_used];
					(*reg_map)[least_used] = -1;

					// update instr's info
					(*reg_map)[instr] = reg;
					spilled->insert(least_used);
									// instr, and set target to be spill
				}
			}



		}
		index++;
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


	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_num_uses = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> *block_to_live_range = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_reg_map = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	
	// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
// ================
// COMPUTE LIVENESS
// ================
			
			printf("\n\n == COMPUTING LIVENESS == \n\n");
			
			// initialize two mappings for the given block
			std::unordered_map<LLVMValueRef,int> *num_uses = new std::unordered_map<LLVMValueRef,int> ();
			std::unordered_map<LLVMValueRef, int[2]> *live_range = new std::unordered_map<LLVMValueRef, int[2]> ();
		
		
			// update live_range for this block
			compute_liveness(basicBlock,num_uses,live_range);
			
			// assign mapping for block-level
			(*block_to_live_range)[basicBlock] = live_range;
			(*block_to_num_uses)[basicBlock] = num_uses;
// DEBUG:			
			print_umap(live_range);

// ==========================
// ACTUAL REGISTER ALLOCATION
// ==========================
			
			// DEBUG
			printf("\n\n == REGISTER ALLOCATION == \n\n");

			// reg_map, a mapping from each instruction to each physical register
			std::unordered_map<LLVMValueRef, int> *reg_map = new std::unordered_map<LLVMValueRef,int> ();
			
			// get register allocations for this block
			register_allocate(basicBlock,live_range,reg_map,num_uses,registers_available);

			(*block_to_reg_map)[basicBlock] = reg_map;
	
			// DEBUG
			printf("\n\n printing reg_map:\n");
			for (auto itr : *reg_map) {	
				LLVMDumpValue(itr.first); fflush(stdout);
				printf(" -> %d\n",itr.second);
			}

			
		}
	}

	return 0;

}
