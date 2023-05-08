#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cassert>

#include "optimizer.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#define prt(x) if(x) { printf("%s\n", x); }

int const_fold(LLVMModuleRef module);

LLVMModuleRef createLLVMModel(char * filename){
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

void walkBBInstructions(LLVMBasicBlockRef bb){
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction;
  				instruction = LLVMGetNextInstruction(instruction)) {
	
		//LLVMGetInstructionOpcode gives you LLVMOpcode that is a enum		
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
        
		
		//if (op == LLVMCall) //Type of instruction can be checked by checking op
		//if (LLVMIsACallInst(instruction)) //Type of instruction can be checked by using macro defined IsA functions
		//{
		

	}

}

void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		const char* funcName = LLVMGetValueName(function);	

		printf("\nFunction Name: %s\n", funcName);

		//walkBasicblocks(function);
 	}
}



void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		//printf("\nIn basic block\n");
		walkBBInstructions(basicBlock);
	
	}
}



/* constant folding
		goes over all instructions and finds instructions
		where opcode corresponds to arithmetic operations
		(+,-,*) and all operands are constants

	- using LLVMConst" and LLVMReplaceAllUsesWith
		where " is
			- Add
			- Sub
			- Mul

	- returns
			- 0 if no changes were made
			- any integer > 0 - the # of changes made
*/
int const_fold(LLVMModuleRef module) {
	int changes_made = 0;
//	printf("we do a little constant folding\n");

	// for each function
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
				// get the opcode
				LLVMOpcode opcode = LLVMGetInstructionOpcode(instr);

				// if opcode is +, -, or *
				if ((opcode == LLVMAdd) || (opcode == LLVMSub) || (opcode == LLVMMul)) {
					LLVMValueRef operand1 = LLVMGetOperand(instr,0);
					LLVMValueRef operand2 = LLVMGetOperand(instr,1);

					if (LLVMIsConstant(operand1) && LLVMIsConstant(operand2)) {
						switch (opcode) {
							case LLVMAdd:
								LLVMReplaceAllUsesWith(instr,LLVMConstAdd(operand1,operand2));
								changes_made++;

							case LLVMSub:
								LLVMReplaceAllUsesWith(instr,LLVMConstSub(operand1,operand2));
								changes_made++;
						
							case LLVMMul:
								LLVMReplaceAllUsesWith(instr,LLVMConstMul(operand1,operand2));
								changes_made++;

						}	
					}
				}				
			}
		}

	}
	return changes_made;
}

/* common sub-expressions
	goes through instructions in each basic block
	and identifies pairs of intructions with same
	opcode and operands

	safety check needed if they're load operations
	
	using LLVMReplaceAllUsesWith


	- returns
			- 0 if no changes were made
			- any integer > 0 - the # of changes made

*/
int com_subexp(LLVMModuleRef module) {
	int changes_made = 0;
//	printf("we do a little common sub-expressions\n");
	
	// for each function
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {		

				// store curr instruction's opcode and operands
				LLVMOpcode opcode = LLVMGetInstructionOpcode(instr);
				LLVMValueRef operand1 = LLVMGetOperand(instr,0);
				//printf("\n%s\n",LLVMPrintValueToString(instr));
					
				// if it's a load
				if (opcode == LLVMLoad) {
					for (LLVMValueRef other_instr = LLVMGetNextInstruction(instr); other_instr; other_instr = LLVMGetNextInstruction(other_instr)) {
						
						// if we find a store that affects it, we cancel and move on 
						if ((LLVMGetInstructionOpcode(other_instr) == LLVMStore) && (LLVMGetOperand(other_instr,1) == operand1)) {
							// we found a store to the same memory location
							// we cannot optimize this instr further
							continue;
							
						}
						else if ((LLVMGetInstructionOpcode(other_instr) == LLVMLoad) && LLVMGetOperand(other_instr,0) == operand1) {
							LLVMReplaceAllUsesWith(other_instr,instr);
							changes_made++;
						}
					}
				}

				// if it's an arithmetic expression where order doesn't matter
				else if ((LLVMAdd == opcode) ||  (LLVMMul == opcode)) {
					LLVMValueRef operand2 = LLVMGetOperand(instr,1);
					for (LLVMValueRef other_instr = LLVMGetNextInstruction(instr); other_instr; other_instr = LLVMGetNextInstruction(other_instr)) {
						// if same code and same operands (order doesn't matter)
						if ((opcode == LLVMGetInstructionOpcode(other_instr)) && (((operand1 == LLVMGetOperand(other_instr,1)) && (operand2 == LLVMGetOperand(other_instr,0))) || ((operand1 == LLVMGetOperand(other_instr,0)) && (operand2 == LLVMGetOperand(other_instr,1))))) {
							
							// replace uses and update
							LLVMReplaceAllUsesWith(other_instr,instr);
							changes_made++;
						}						
					}
				}

				// if it's an arthmetic expression where order matters
				else if ((LLVMSub == opcode) || (LLVMUDiv == opcode) || (opcode == LLVMFDiv)) {
					LLVMValueRef operand2 = LLVMGetOperand(instr,1);
					for (LLVMValueRef other_instr = LLVMGetNextInstruction(instr); other_instr; other_instr = LLVMGetNextInstruction(other_instr)) {

						// if same code and same operands in order
						if ((opcode == LLVMGetInstructionOpcode(other_instr)) && ((operand1 == LLVMGetOperand(other_instr,0)) && (operand2 == LLVMGetOperand(other_instr,1)))) {

							// replace uses and update
							LLVMReplaceAllUsesWith(other_instr,instr);
							changes_made++;
						}
					}
				}
			}
		}
	}
	return changes_made;
}

/* removes useless instructions

- we don't mess with store
- we use LLVMInstructionEraseFromParent


	- returns
			- 0 if no changes were made
			- any integer > 0 - the # of changes made

*/
int dead_code(LLVMModuleRef module) {
	int changes_made = 0;
//	printf("we do a bit of dead code removal\n");

	// for each function
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
				// get opcode
				LLVMOpcode opcode = LLVMGetInstructionOpcode(instr);
				
				
//				printf("\n%s\n",LLVMPrintValueToString(instr));	
				int num_of_operands = LLVMGetNumOperands(instr);

//				printf("\topcode: %d\n",opcode);
		
				// if it's not alloc, return, or store, br, or return or a call
				if ((opcode != LLVMAlloca) && (opcode != LLVMRet) && (opcode != LLVMStore) && (opcode != LLVMRet) && (opcode != LLVMBr) && (opcode != LLVMCall))	{
					
					int mentioned = 0;
					
					// now we go through all the lines below our target line in the
					// same block
					for(LLVMValueRef other_instr = LLVMGetNextInstruction(instr); other_instr; other_instr = LLVMGetNextInstruction(other_instr)) {
						
						// go through for references to our instruction	
						for (int j = 0; j < LLVMGetNumOperands(other_instr); j++) {
							if (instr == LLVMGetOperand(other_instr,j)) { mentioned=1;}
						}
					
					}
					// if no references found
					if (mentioned == 0) {
						LLVMInstructionEraseFromParent(instr);
						changes_made++;
					}
				}
			}
		}
	}

	return changes_made;
}

// not due this week
int const_prop(LLVMModuleRef module) {
	int changes_made = 0;
	//printf("constant propogation!\n");
	return changes_made;
}




int optimize(char* filename) {
	assert(filename != NULL);

	LLVMModuleRef module;
	
	module = createLLVMModel(filename);	
	
	if (module == NULL) {
		fprintf(stderr,"module is NULL!\n");
		return -1;
	}
	
	int con_fold_changes, com_subexp_changes, dead_code_changes, con_prop_changes;
	//LLVMPrintModuleToFile(module,"before",NULL);
	int i = 1;
	do {
		printf("\n");	
		// constant folding
		con_fold_changes = const_fold(module);
		printf("Iteration %d: %d constant folding changes.\n",i,con_fold_changes);
		
		dead_code_changes = dead_code(module);
		printf("Iteration %d: %d dead code changes\n",i,dead_code_changes);

		com_subexp_changes = com_subexp(module);
		printf("Iteration %d: %d common subexpression changes\n",i,com_subexp_changes);
		//LLVMDumpModule(module);
		i++;	
	} while(con_fold_changes + dead_code_changes + com_subexp_changes != 0);
	
	//LLVMPrintModuleToFile(module,"after",NULL);


	return 0;
}

