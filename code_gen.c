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

void expire_old_intervals(int index, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<int,LLVMValueRef> *active, std::set<int> *free,std::set<LLVMValueRef> *spilled);
void spill (LLVMValueRef instr, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses,std::unordered_map<int,LLVMValueRef> *active,std::set<LLVMValueRef> *spilled);
int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef, int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses, int registers_available,std::set<LLVMValueRef> *spilled);
void gen_offset_map(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef, int> *offset_map, std::set<LLVMValueRef> *spilled, int offset);
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

		look at the registers being used right now by
		looking at active, and by seeing how active the value is
		(by looking at live_range), this function expires intervals that have 
		expired and updates active and free.

		Inputs
			index 		- the index of the instruction we're currently on
			live_range 	- mapping from instruction to pair of integers, the (start,stop)
						  which captures which indices the value is alive for
			active		- mapping from register (int) to an instruction, to capture which registers
					   	  are currently active (in use)
			free		- set of registers (integers) to indicate that they're free
			spilled		- set of instructions currently spilled
*/

void expire_old_intervals(int index, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<int,LLVMValueRef> *active, std::set<int> *free,std::set<LLVMValueRef> *spilled) {
	
	int reg;

	// expire old intervals, by looping through each active interval
		std::unordered_map<int,LLVMValueRef>::iterator itr;

		// see if any intervals have expired
		// so for each active interval
		for (itr = active->begin(); itr != active->end(); )	{
			
//			printf("Expiring intervals!\n");

			// if we are at or past the interval, free the register
			if ((*live_range)[itr->second][1] < index) {
	
// NOTE: IF YOU CAN EXPIRE AND USE THE SAME REGISTER ON A GIVEN LINE
// AND YOU NEED TO CHECK CONDITIONS (like if it's the first operand
// in the add or smt), THE CHECK WILL BE HERE. USE instr and active_itr.
		// but make sure you dont break the iterator, we're manually
		// incrementing

	
//				printf("Index %d: expiring register %d because it expired at %d\n",index,itr->first,(*live_range)[itr->second][1]);

				// swap out registers 
				reg = itr->first;              // store register in tmp integer
				itr = active->erase(itr);	   // erase from active
				free->insert(reg);			   // add it to free registers set
		

			} else ++itr;
		}

}

/*
 *	If all registers are full, we spill by assigning
 *	a vallue to memory instead, it'll have a register of 
 *	-1
 *
 *  Spill is based on the heuristic of choosing the
 *  value with the least memory lookups, to minimize
 *  computation time
 *
 * 	Inputs
 * 		instr	- instruction that needs a register
 * 		reg_map - mapping from instructions to registers
 * 		num_uses- number of uses for each instruction
 * 		active  - mapping from registers to instructions
 * 		spilled - set of instructions that are spilled
 * 		 
 */
void spill (LLVMValueRef instr, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses,std::unordered_map<int,LLVMValueRef> *active,std::set<LLVMValueRef> *spilled) {
	// pick which one to spill

	LLVMValueRef least_used = instr;
	int least_uses = (*num_uses)[instr];				

	int reg;

	// go through all used instructions right now, and
	// get the one with the least memory look ups
	std::unordered_map<int,LLVMValueRef>::iterator itr;
		for (itr = active->begin(); itr != active->end(); ++itr)	{
			
			
			// if it is used less		
			if ( (*num_uses)[itr->second] < least_uses ) {

/* //DEBUG
				printf("New least used, with only %d uses: \t",least_uses); fflush(stdout);
				LLVMDumpValue(itr->second); fflush(stdout);
				printf("\n"); fflush(stdout);
*/

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

		// update replacement
		reg = (*reg_map)[least_used];
		(*reg_map)[least_used] = -1;

		// update instr's info
		(*reg_map)[instr] = reg;
		spilled->insert(least_used);
		// instr, and set target to be spill
	}
}


/*
	Populates a mapping between instructions and registers
	as a way of allocating them
	
	Registers are represented as integers (starting from 0 and going up)

	Inputs
		basicBlock - LLVMBasicBlockRef to access instructions
		live_range - map form instruction to integer array of size 2 (start, stop)
					 this is expected to be set up previously by compute_liveness
		reg_map	   - map from instruction to instruction, this will be updated in
					 this function
		num_uses   - map from instruction to integer counting the number of uses
					 for each given instruction. Also made by compute_liveness
		registers_available 	- just an integer for the # of registers available


*/
int register_allocate(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int[2]> *live_range, std::unordered_map<LLVMValueRef, int> *reg_map, std::unordered_map<LLVMValueRef,int> *num_uses, int registers_available,std::set<LLVMValueRef> *spilled) {	
	int index = 0;

	int reg;
	
	// set of free registers
	std::set<int> *free = new std::set<int>();
	// map from register int and instruction that it holds for
	std::unordered_map<int,LLVMValueRef> *active = new std::unordered_map<int,LLVMValueRef>();	



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
		expire_old_intervals(index, live_range, active, free,spilled);	
		

		// if we have a start point on this instruction (if there's an interval)
		if (live_range->find(instr) != live_range->end()) {
			

			// if there's a free register
			if (free->size() > 0) {

				// get a free register	
				reg = *(free->begin());

/* DEBUG:	
				printf("Using register %d for instr: ",reg); fflush(stdout);
				LLVMDumpValue(instr); fflush(stdout);
				printf("\n");	
*/

				// update free and active
				free->erase(reg);
				(*active)[reg] = instr;
				
				(*reg_map)[instr] = reg;	

			}
			// if no registers available, we spill
			else {
				
				spill(instr,reg_map,num_uses,active,spilled);
			}

		} 
		index++;
	}
	delete free;
	delete active;

	return 0;
}


/*
 *	Prints the push calls to 
 *	set up the stack frame
 *  properly
 *
 */
void printDirectives() {
	
	printf("\tpush %%ebp\n");				 // save the base pointer 
	printf("\tmov %%esp, %%ebp\n");		 	 // update ebp
	printf("\tsub $4, %%esp\n"); 			 // room for 1 variable
}

/*
 *  Prints the pop calls to
 *  revert to the previous stack
 *  frame properly
 *
 */
void printFunctionEnd() {

	printf("\tmov %%ebp, %%esp\n");         // update esp
	printf("\tpop %%ebp\n");				// recover old ebp
	printf("\tret\n");
}
/*
 *
 *
 *
 *
 */
void gen_offset_map(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef, int> *offset_map, std::set<LLVMValueRef> *spilled, int offset) {

	int LOCALMEM = -4;
	// loop through instructions
	for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {

		// LLVMDumpValue(instr);fflush(stdout);printf("\n");fflush(stdout);
		// if it's an alloc, update offset map
		if (LLVMGetInstructionOpcode(instr) == LLVMAlloca) {

			(*offset_map)[instr] = offset;
			offset = offset + LOCALMEM;
		}

		else if (spilled->find(instr) != spilled->end()) {
			
			LLVMValueRef hopefully_allocd;

			// if the instruction is a load, easy, we pray that we loaded from
			// an alloc'd variable
			if (LLVMGetInstructionOpcode(instr) == LLVMLoad) {

				// if we're working with an alloc'd variable
				hopefully_allocd = LLVMGetOperand(instr,0);
					
				if (LLVMGetInstructionOpcode(hopefully_allocd) == LLVMAlloca) {

					assert(offset_map->find(hopefully_allocd) != offset_map->end());

					(*offset_map)[instr] = (*offset_map)[hopefully_allocd];

					continue;
				}
			}

			// search the rest of block for a store
			// and pray we store to an alloc'd variablwe
			for (LLVMValueRef next_instr = instr; next_instr; next_instr = LLVMGetNextInstruction(next_instr)) {
				
				if (LLVMGetInstructionOpcode(next_instr) == LLVMStore)  {

					// AND if we're working with an alloc'd variable
					hopefully_allocd = LLVMGetOperand(next_instr,1);

					// if we store to an alloc'd variable
					if ((LLVMGetOperand(next_instr,0) == next_instr) && (LLVMGetInstructionOpcode(hopefully_allocd) == LLVMAlloca)) {
						assert(offset_map->find(hopefully_allocd) != offset_map->end());

						(*offset_map)[instr] = (*offset_map)[hopefully_allocd];

						continue;
					}
				}
			}

			// if we make it here and we didn't find a load or store
			// we'll just allocate our own memory
			if (offset_map->find(instr) == offset_map->end()) {

				(*offset_map)[instr] = offset;
				offset = offset + LOCALMEM;
			}

			assert(offset_map->find(instr) != offset_map->end());	
		}
	}
}

int code_gen(char* filename) {
	assert(filename != NULL);

	setvbuf(stdout, NULL, _IONBF, 0); // no more buffering
									  //
	LLVMModuleRef module;
	
	int registers_available = 2;
	
	module = importLLVMModel(filename);	
	
	if (module == NULL) {
		fprintf(stderr,"module is NULL!\n");
		return -1;
	}


	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_num_uses = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> *block_to_live_range = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int[2]>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_reg_map = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	// to go from integer to register, we have this map
	std::unordered_map<int,char *> *int2reg = new std::unordered_map<int, char *>();
	char ebx[4] = "ebx";
	char ecx[4] = "ecx";
	char edx[4] = "edx";	
	(*int2reg)[0] = ebx;
	(*int2reg)[1] = ecx;
	(*int2reg)[2] = edx;

	
	printf("\n\n============ ASSEMBLY OUTPUT =============\n\n");

	// calling our 1 function main
	printf("main:\n");
	// PRINT DIRECTIVES
	printDirectives();

	// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
		// offset map needs to be cross-block
		std::unordered_map<LLVMValueRef, int> *offset_map = new std::unordered_map<LLVMValueRef, int>();
		int offset = -4;

		// doing createBBLabels within the for loops
		std::unordered_map<LLVMBasicBlockRef, char *> *block_labels = new std::unordered_map<LLVMBasicBlockRef, char *>();
		int block_count = 0;

		
		
		// SET UP basicblock labels

		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

			// create BBLabel
			char* block_label = (char*)malloc(8);
			sprintf(block_label,"bb%d",block_count);
			(*block_labels)[basicBlock] = block_label;
			block_count++;

		}



		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
// ================
// COMPUTE LIVENESS
// ================

// DEBUG:
//			printf("\n\n == COMPUTING LIVENESS == \n\n");
			
			// initialize two mappings for the given block
			std::unordered_map<LLVMValueRef,int> *num_uses = new std::unordered_map<LLVMValueRef,int> ();
			std::unordered_map<LLVMValueRef, int[2]> *live_range = new std::unordered_map<LLVMValueRef, int[2]> ();
		
			// update live_range for this block
			compute_liveness(basicBlock,num_uses,live_range);
			
			// assign mapping for block-level
			(*block_to_live_range)[basicBlock] = live_range;
			(*block_to_num_uses)[basicBlock] = num_uses;

/*  //DEBUG:			
			printf("\n\nLIVENESS RANGES\n");
			for (auto itr : *live_range) {	
				LLVMDumpValue(itr.first);
				printf(" -> (%d, %d) \n ", itr.second[0], itr.second[1]); 
			}
*/	
// ==========================
// ACTUAL REGISTER ALLOCATION
// ==========================
			

			// set of instructions that are currently spilled
			std::set<LLVMValueRef> *spilled = new std::set<LLVMValueRef>();


			// reg_map, a mapping from each instruction to each physical register
			std::unordered_map<LLVMValueRef, int> *reg_map = new std::unordered_map<LLVMValueRef,int> ();
			
			// get register allocations for this block
			register_allocate(basicBlock,live_range,reg_map,num_uses,registers_available,spilled);
			
			(*block_to_reg_map)[basicBlock] = reg_map;
	
/* // DEBUG
			printf("\n\nREGISTER ALLOCATION\n");
			for (auto itr : *reg_map) {	
				LLVMDumpValue(itr.first); fflush(stdout);
				printf(" -> %d\n",itr.second);
			}
*/

// ===================
// GENERATE OFFSET MAP
// ===================

			gen_offset_map(basicBlock, offset_map, spilled,offset);

			
/*			// DEBUG:
			// print offset map
			printf("\n\nOFFSET MAP\n");
			for (auto itr : *offset_map) {	
				LLVMDumpValue(itr.first); fflush(stdout);
				printf(" -> %d\n",itr.second);
			}
*/
			
// ======================
// Generate Assembly Code
// ======================

			printf("\n\n");
			
			printf("%s:\n",(*block_labels)[basicBlock]);


			
			// initialize offset_map and localMem

			char* to_write = (char *)malloc(16);
			char reg[4];
			// not doing any initialization lol
			// but we're not pushing anything maybe?
			
			LLVMValueRef A;
			LLVMValueRef B;
			
			long long value;

			int offset0;
			int offset1;
			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {

				LLVMOpcode opcode = LLVMGetInstructionOpcode(instr);

			// RET
				// based on each opcode
				if (opcode == LLVMRet) {

					A = LLVMGetOperand(instr,0);
					// if it returns void
					
					if (LLVMGetNumOperands(instr) == 0) {
						// nothing to be done

					}

					// if A is constant
					else if (LLVMIsConstant(A)) {
													
						value = LLVMConstIntGetSExtValue(A);
						printf("\tmovl $%lld, %%eax\n",value);
					}

					// if A is tmp variable (in memory)
					else if (offset_map->find(A) != offset_map->end()) {
	

						offset0 = (*offset_map)[A];
						printf("\tmovl %d(%%ebp), %%eax\n",offset0);
					}	
					// if A is tmp variable (in register)
					else {

						assert(reg_map->find(A) != reg_map->end());

						to_write = (*int2reg)[ (*reg_map)[A] ];
						printf("\tmovl %%%s, %%eax\n",to_write);
					}


					// print end of function
					printFunctionEnd();

				
				}
			// LOAD
				// %a = load i32, i32* %b 
				else if (opcode == LLVMLoad) {
					
					B = LLVMGetOperand(instr,0);
					assert(offset_map->find(B) != offset_map->end());
					assert(reg_map->find(instr) != reg_map->end());
						
					offset0 = (*offset_map)[B];
					to_write = (*int2reg)[ (*reg_map)[instr] ];

					printf("\tmovl %d(%%ebp), %%%s\n",offset0,to_write);
				}
			// STORE
				// store i32 A, i32^ %b
				else if (opcode == LLVMStore) {
					
					A = LLVMGetOperand(instr,0);
					B = LLVMGetOperand(instr,1);
					
					// if A is a parameter (it was used by a Call)
					//
					assert(LLVMGetFirstUse(A) != NULL);

					if (LLVMGetInstructionOpcode(LLVMGetUsedValue(LLVMGetFirstUse(A))) == LLVMCall) {

						// we ignore parameters
					}
					// if A is a constant
					else if (LLVMIsConstant(A)) {
						

						assert(offset_map->find(B) != offset_map->end());
				
						value = LLVMConstIntGetSExtValue(A);
						offset0 = (*offset_map)[B];

						printf("\tmovl $%lld, %d(%%ebp)\n",value,offset0);
					}
					// if A is a temp var (in register)
					else if ((reg_map->find(instr) != reg_map->end()) && ((*reg_map)[instr] != -1)) {

						assert(reg_map->find(A) != reg_map->end());
						assert(offset_map->find(B) != offset_map->end());
						
						offset0 = (*offset_map)[B];
						to_write = (*int2reg)[ (*reg_map)[A] ];

						printf("\tmovl %%%s %d(%%ebp)\n",to_write,offset0);
					}
					// if A is a temp var (spilled)
					else {
					
						// EDGE CASE, store %0
						// we %0 is a parameter for this function
						//
						// and since mini C only takes 1 parameter, there's only 1 possibility
						if (offset_map->find(A) == offset_map->end()) {

							printf("\tmovl 4(%%ebp), %%eax\n");
						} else {

							offset0 = (*offset_map)[A];
							printf("\tmovl %d(%%ebp), %%eax\n",offset0);

						}
						
						assert(offset_map->find(B) != offset_map->end());
						
						offset1 = (*offset_map)[B];
						printf("\tmovl %%eax, %d(%%ebp)\n",offset1);

					}

					// if A is a parameter, ignore anyway
				}
			// CALL
				// %a = call type @func(P) or call type @func(P)
				else if (opcode == LLVMCall) {
					
					// save gen purpose registers
					printf("\tpushl %%ebx\n\tpushl %%ecx\n\tpushl %%edx\n");
					
					LLVMValueRef func = LLVMGetOperand(instr,0);

					assert(LLVMCountParams(func) < 2);

					// if function has a parameter
					if (LLVMCountParams(func) == 1) {
						
						A = LLVMGetParam(func,0);

						// if the param is a constant
						if (LLVMIsConstant(A)) {

							value = LLVMConstIntGetSExtValue(A);
							printf("\tpushl $%lld\n",value);
						}
						// if param is a tmp var (in reg)
						else if ((reg_map->find(A) != reg_map->end()) && ((*reg_map)[instr] == -1)) {

							to_write = (*int2reg)[ (*reg_map)[A] ];

							printf("\tpushl %%%s\n",to_write);

						} 
						// if param is a tmp (in memory)
						else {
							
							assert(offset_map->find(A) != offset_map->end());

							offset0 = (*offset_map)[A];
							printf("\tpushl %d(%%ebp)\n",offset0);
						}
					}


					printf("\tcall %s\n",LLVMGetValueName(func)); 

					// if the function had a parameter, we undo a push
					if (LLVMCountParams(func) == 1) {

						printf("\taddl $4, %%esp\n");
					}

					printf("\tpopl %%edx\n\tpopl %%ecx\n\tpopl %%ebx\n");

					// if function is of type %a = call 
					// and is in a register
					if ((reg_map->find(instr) != reg_map->end()) && ((*reg_map)[instr] != -1)) {
					
						to_write = (*int2reg)[ (*reg_map)[instr] ];
						printf("\tmovl %%eax, %%%s\n",to_write);

					}
					else if (offset_map->find(instr) != offset_map->end()) {

						value = LLVMConstIntGetSExtValue(instr);
						printf("\tmovl %%eax, %lld(%%ebp)\n",value);
					}
				}
			// BREAK
				// br i1 %a, label %b, label %c OR br label %b
				else if (opcode == LLVMBr) {
					
					LLVMBasicBlockRef block0 = LLVMValueAsBasicBlock(LLVMGetOperand(instr,0));
					LLVMBasicBlockRef block1;
					
					// if the branch is unconditional
					if (!LLVMIsConditional(instr)) {

						block0 = LLVMValueAsBasicBlock(LLVMGetOperand(instr,0));	
						
						assert(block_labels->find(block0) != block_labels->end());

						printf("\tjmp %s\n",(*block_labels)[block0]);
					}
					// if it is conditional
					else {

						LLVMValueRef cond = LLVMGetCondition(instr);

						block0 = LLVMValueAsBasicBlock(LLVMGetOperand(instr,1));
						block1 = LLVMValueAsBasicBlock(LLVMGetOperand(instr,2));
					
						assert(block_labels->find(block0) != block_labels->end());
						assert(block_labels->find(block1) != block_labels->end());

						LLVMIntPredicate pred = LLVMGetICmpPredicate((cond));
		
/*						NOTE ABOUT ORDERING OF OPERANDS IN PREDICATES
									between llvm-ir and x86

							// operand ordering for llvm-ir is different from x86
							// we gotta swap
								// e.g. icmp sgt op1 op2, will b true if op1 >= op2
								// e.g. cmp %ebx %eax; jge block, will jump if %eax >= %ebx
		
								// but we let the condition branch handle
								// producing cmp %ebx %eax, so 

*/

						// find out what the predicate is
						switch (pred) {
							
							case LLVMIntEQ:

								printf("\tje %s\n",(*block_labels)[block0]);
								break;

							case LLVMIntNE:

								printf("\tjne %s\n",(*block_labels)[block0]);
								break;

							case LLVMIntSGT: // SWAPPED

								printf("\tjl %s\n",(*block_labels)[block0]);
								break;
	
							case LLVMIntSGE: // SWAPPED

								printf("\tjle %s\n",(*block_labels)[block0]);
								break;

							case LLVMIntSLT: // SWAPPED

								printf("\tjg %s\n",(*block_labels)[block0]);
								break;

							case LLVMIntSLE:
								
								printf("\tjge %s\n",(*block_labels)[block0]);
								break;

							default:
								assert(1!=1);
					
						}
						printf("\tjmp %s\n",(*block_labels)[block1]);
					}

				}
				// %a = add/mul/sub nsw A,B or %a = icmp slt A,B
				else if ( (opcode == LLVMAdd) || (opcode == LLVMSub) || (opcode == LLVMMul) || (opcode == LLVMICmp) ) {
					
					A = LLVMGetOperand(instr,0);
					B = LLVMGetOperand(instr,1);
						
					// if curr line is a tmp var
					if (reg_map->find(instr) != reg_map->end()) {
						
						// if spilled
						if ((*reg_map)[instr] == -1) {
							sprintf(to_write,"eax");
						}	
						// if in regis
						else {
							assert(int2reg->find((*reg_map)[instr]) != int2reg->end());
							to_write = (*int2reg)[ (*reg_map)[instr] ];				
						}
					}

					
					// if A is constant
					if (LLVMIsConstant(A)) {

						value = LLVMConstIntGetSExtValue(A);
						printf("\tmovl $%lld, %%%s\n",value,to_write);
					}
					// if A is a tmp var (in regis)
					else if ((reg_map->find(A) != reg_map->end()) && ((*reg_map)[A] != -1)) {


						printf("\tmovl %%%s, %%%s\n",(*int2reg)[ (*reg_map)[A] ], to_write);
					}
					// if A is a tmp var (spilled)
					else if (offset_map->find(A) != offset_map->end()) {

						assert(offset_map->find(A) != offset_map->end());

						offset0 = (*offset_map)[A];
						printf("\tmovl %d(%%ebp), %%%s\n",offset0, to_write);
					}
					else assert(2>3);

					// print the opcode first.
					switch (opcode) {

						case LLVMAdd:
							printf("\tadd");
							break;

						case LLVMSub:
							printf("\tsub");
							break;

						case LLVMMul:
							printf("\timul");
							break;

						case LLVMICmp:
							printf("\tcmp");
							break;


								
					}
					// if B is constant
					if (LLVMIsConstant(B)) {

						value = LLVMConstIntGetSExtValue(B);
						printf(" $%lld, %%%s\n", value,to_write);
					}
					// if B is a tmp var
					else if (reg_map->find(A) != reg_map->end()) {
							
						// if spilled
						if ((*reg_map)[A] == -1) {
							offset0 = (*offset_map)[A];
							printf(" %d(%%ebp), %%%s\n",offset0, to_write);

						} 
						// if in register
						else printf(" %%%s, %%%s\n", (*int2reg)[ (*reg_map)[A] ], to_write);
					}
					// if B is a tmp var (spilled)
					else if (offset_map->find(A) != offset_map->end()) {

						offset0 = (*offset_map)[A];
						printf(" %d(%%ebp), %%%s\n",offset0, to_write);
					}
					else (assert(3>4));


					// if %a is in memory (register)
					if ((reg_map->find(instr) != reg_map->end()) && ((*reg_map)[instr] != -1)) {

						printf("\tmovl %%%s, %%%s\n", to_write, (*int2reg)[(*reg_map)[instr]]);

					}
					// if %a is spilled
					else if (offset_map->find(instr) != offset_map->end()) {

						offset0 = (*offset_map)[instr];
						printf("\tmovl %%%s %d(%%ebp)\n",to_write, offset0);
					}
							
							
				}
						
			}


		// clean up
			delete num_uses;
			delete reg_map;	
		}
		delete offset_map;
	}

	return 0;

}
