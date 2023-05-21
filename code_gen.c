#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cassert>

#include <algorithm>
#include <set>
#include <unordered_map>

#include "code_gen.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#define prt(x) if(x) { printf("%s\n", x); }

int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *list_index, std::unordered_map<LLVMValueRef, std::pair<int, int>*> *live_range);



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
		list_index - a mapping from an instruction to their index in the block (an integer starting at 0)
		live_range - mapping from an instruction to a pair of integers
*/
int compute_liveness(LLVMBasicBlockRef basicBlock, std::unordered_map<LLVMValueRef,int> *list_index, std::unordered_map<LLVMValueRef, std::pair<int, int>*> *live_range) {

	// for each instruction

		// if there's a new value (arithmetic or load i think?)

		// spam LLVMGetNextUse until we get NULL, and then that's the last use of that value
		// map the life span and head out	

	
	return 0;
}

int code_gen(char* filename) {
	assert(filename != NULL);

	LLVMModuleRef module;
	
	module = importLLVMModel(filename);	
	
	if (module == NULL) {
		fprintf(stderr,"module is NULL!\n");
		return -1;
	}

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> *block_to_list_index = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, int>*> ();

	std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, std::pair<int,int>*>*> *block_to_live_range = new std::unordered_map<LLVMBasicBlockRef, std::unordered_map<LLVMValueRef, std::pair<int,int>*>*> ();

	
	// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

			std::unordered_map<LLVMValueRef,int> *list_index = new std::unordered_map<LLVMValueRef,int> ();
			std::unordered_map<LLVMValueRef, std::pair<int, int>*> *live_range = new std::unordered_map<LLVMValueRef, std::pair<int, int>*> ();
			

			compute_liveness(basicBlock,list_index,live_range);




		}
	}

	return 0;

}
