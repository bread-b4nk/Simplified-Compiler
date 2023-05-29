#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cassert>

#include <algorithm>
#include <set>
#include <unordered_map>

#include "optimizer.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#define prt(x) if(x) { printf("%s\n", x); }

int const_fold(LLVMModuleRef module);

// helper functions for constant propogation
void make_gen(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_gen, std::set<LLVMValueRef> *all_stores);
void make_kill(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_kill, std::set<LLVMValueRef> *all_stores);
void make_in_out(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_in, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_out, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_gen, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_kill);
int actually_optimize(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_in);



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
			//	printf("\n%s\n",LLVMPrintValueToString(instr));
					
				// if it's a load
				if (opcode == LLVMLoad) {
					for (LLVMValueRef other_instr = LLVMGetNextInstruction(instr); other_instr; other_instr = LLVMGetNextInstruction(other_instr)) {
						
						// if we find a store that affects it, we cancel and move on 
						if ((LLVMGetInstructionOpcode(other_instr) == LLVMStore) && (LLVMGetOperand(other_instr,1) == operand1)) {
							// we found a store to the same memory location
							// we cannot optimize this instr further
							continue;
							
						} // otherwise, if we find a load from the same place
						else if ((LLVMGetInstructionOpcode(other_instr) == LLVMLoad) && LLVMGetOperand(other_instr,0) == operand1) {
							LLVMReplaceAllUsesWith(other_instr,instr);
							changes_made++;
						}
					}
				}

				// if it's an arithmetic expression where order doesn't matter (+ or *)
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

				// if it's an arthmetic expression where order matters (- or /)
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
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr ; instr = LLVMGetNextInstruction(instr)) {
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


/*			helper function for constant propogation
	Makes a GEN set for each basic block and creates an unordered_map to access each one.
			

	Inputs
		module - LLVMModuleRef to access LLVM-IR code
		big_gen - mapping from basic blocks to sets of LLVMValueRefs, which is each GEN set
		all_stores - set of all stores to generate KILL later


*/
void make_gen(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_gen, std::set<LLVMValueRef> *all_stores) {

		// for each function (all 1 of them)
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
			std::set<LLVMValueRef> *gen = new std::set<LLVMValueRef> ();
		
			// add it to the big GEN
			//big_gen->insert(std::make_pair(basicBlock,gen));
			(*big_gen)[basicBlock] = gen;
			
			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {

				// if it's a store instruction
				LLVMOpcode opcode = LLVMGetInstructionOpcode(instr);
							

				if (opcode == LLVMStore) {
//					printf("\tFound a store!\n");
					// get target memory address
					LLVMValueRef target = LLVMGetOperand(instr,1);	
							
					// check if there's another store to the same location, if there is, delete it
					std::set<LLVMValueRef>::iterator itr;
					for (itr = gen->begin(); itr != gen->end(); itr++) {
						if (target == LLVMGetOperand(*itr,1)) {
//							printf("\tRemoving store\n");
							gen->erase (*itr);
						}
					}

					// add the current store instruction to gen
					// and set of all stores
					gen->insert(instr);
					all_stores->insert(instr);
				}				
			}
/*			
			printf("Final gen:\n");
			std::set<LLVMValueRef>::iterator itr;
			for (itr = gen->begin(); itr != gen->end(); itr++) {
				printf("%s\n",LLVMPrintValueToString(*itr));
			}
*/
		}
	}
}

/*	        helper function for constant propogation
	Makes the KILL set for each basic block

*/
void make_kill(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_kill, std::set<LLVMValueRef> *all_stores) {

		// for each function (we have only 1 lol)
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

			// produce KILL set
			std::set<LLVMValueRef> *kill = new std::set<LLVMValueRef> ();
			
			// add kill set to kill unordered map	
			(*big_kill)[basicBlock] = kill;

			// for each instruction
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
				
				// if it's a store
				if (LLVMGetInstructionOpcode(instr) == LLVMStore) {
					
					// get the piece of memory that we mess with
					LLVMValueRef killed_addr = LLVMGetOperand(instr,1);				
	
					// now we loop through and look for
					// EVERY store killed by this instruction
					std::set<LLVMValueRef>::iterator itr;
					for (itr = all_stores->begin(); itr != all_stores->end(); itr++) {
						
						// if it's not itself, and the same register gets killed
						if ((*itr != instr) && (LLVMGetOperand(*itr,1) == killed_addr)) {
							// add to kill set
							kill->insert(*itr);
						}
					}
				}
			}
/* //DEBUG
			printf("Final kill:\n");
			std::set<LLVMValueRef>::iterator itr;
			for (itr = kill->begin(); itr != kill->end(); itr++) {
				printf("%s\n",LLVMPrintValueToString(*itr));
			}
*/
		}	
	}

}

/*	        helper function for constant propogation
	Uses GEN and KILl to create IN and OUT for each
	basic block

*/
void make_in_out(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_in, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_out, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_gen, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_kill) {

	// unordered_map of predecessors
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>*> *pred_map = new std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>*> ();


	// the line below initializes all sets in pred_map to an empty set
		// (so we can reference them without initializing)
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {(*pred_map)[basicBlock] = new std::set<LLVMBasicBlockRef> ();}}


// initialize all IN[B] and all OUT[B]
	// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			
			

			// set OUT[B] = GEN[B], copying data
			std::set<LLVMValueRef> *gen = ((*big_gen)[basicBlock]);
			std::set<LLVMValueRef> *out = new std::set<LLVMValueRef>(*gen);
			(*big_out)[basicBlock] = out;
		


			// initialize IN[B] to empty set
			(*big_in)[basicBlock] = new std::set<LLVMValueRef> ();


			// now we get successors to update our mapping of predecessors
			// looping through our block's successors
			for (int i = 0; i < LLVMGetNumSuccessors(LLVMGetBasicBlockTerminator(basicBlock)); i++) {
				
				// all successors are predecessors of the current block
				(*pred_map)[LLVMGetSuccessor(LLVMGetBasicBlockTerminator(basicBlock),i)]->insert(basicBlock);
			}
		}	
	}
	
	int	changed = 0;
	while (changed == 0)	{
		changed = 1;	// set changed to false
		
		// for all 1 functions
		for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		
			// for each basic block
			for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
				
				
				// got the set of predecessors
				std::set<LLVMBasicBlockRef> *preds = (*pred_map)[basicBlock];
			
				// union the outs of each predecessor
				std::set<LLVMBasicBlockRef>::iterator itr;
				for (itr = preds->begin(); itr != preds->end(); itr++) {
					(*big_in)[basicBlock]->insert((*big_out)[*itr]->begin(),(*big_out)[*itr]->end());
				}
			
				// oldout = OUT[B]
				std::set<LLVMValueRef> *old_out = (*big_out)[basicBlock];

				// OUT[B] = GEN[B] union (in[B] - kill[B])a
			
				// OUT[B] = IN[B] - KILL[B]
					// done using set_difference
				std::set_difference((*big_in)[basicBlock]->begin(), (*big_in)[basicBlock]->end(),
							 (*big_kill)[basicBlock]->begin(), (*big_kill)[basicBlock]->end(),
							std::inserter(*(*big_out)[basicBlock],(*big_out)[basicBlock]->end()));
				
				// then OUT[B] = OUT[B] union GEN[B]
				std::set<LLVMValueRef>::iterator it;
				for (it = (*big_gen)[basicBlock]->begin(); it != (*big_gen)[basicBlock]->end(); it++) {
					(*big_out)[basicBlock]->insert(*it);
				}
				
				if ((*big_out)[basicBlock] != old_out) {
					changed = 0; //set change to true
				}

/*				 // DEBUG
				printf("In:\n");
				for (it = (*big_in)[basicBlock]->begin(); it != (*big_in)[basicBlock]->end(); it++) {
					printf("%s\n",LLVMPrintValueToString(*it));
				}
				printf("Out:\n");
				for (it = (*big_out)[basicBlock]->begin(); it != (*big_out)[basicBlock]->end(); it++) {
					printf("%s\n",LLVMPrintValueToString(*it));
				}
*/				
			}
		}
	}

	// clean pred_map properly
	for (auto& it : *pred_map) {
  		delete it.second;
	}
	delete pred_map;

}

/*	        helper function for constant propogation
	Uses each IN set to optimize the code
	we return the number of changes made

*/
int actually_optimize(LLVMModuleRef module, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_in) {

	int changes_made = 0;

// for all 1 functions
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function; function = LLVMGetNextFunction(function)) {
		// iterator for later	
		std::set<LLVMValueRef>::iterator itr;

		// for each basic block
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock; basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
			

			// create R
			std::set<LLVMValueRef> *R;
			
			int had_to_create_new_R = 1;

			if ((*big_in)[basicBlock]->size() == 0) { had_to_create_new_R = 0; R = new std::set<LLVMValueRef> (); }
			else { R = (*big_in)[basicBlock]; } //CLONE R
			
			// Create a to-delete set
			std::set<LLVMValueRef> *instr_to_die = new std::set<LLVMValueRef> ();
			
			
			// for every instruction I (instr = I)
			for (LLVMValueRef instr = LLVMGetFirstInstruction(basicBlock); instr; instr = LLVMGetNextInstruction(instr)) {
				
	
				// if it's a store
				if (LLVMGetInstructionOpcode(instr) == LLVMStore) {
						
								
					// remove all store instructions in R that are killed by instruction I
					LLVMValueRef killed_addr = LLVMGetOperand(instr,1);
					
					std::set<LLVMValueRef>::iterator itr;
					
					for (itr = R->begin(); itr != R->end(); ) {
						// if it's not itself, and the same memory gets killed
						if ((*itr != instr) && (LLVMGetOperand(*itr,1) == killed_addr)) {
								itr = R->erase(itr);
						} else ++itr;
					}
					
					// add it to R
					R->insert(instr);
				}
				
				

				// if it's a load
				else if (LLVMGetInstructionOpcode(instr) == LLVMLoad) {


					
					// if R is empty, move onto next instruction I
					if (R->size() == 0) { continue;  }			
	
					// init set of instructions that write to target address
					std::set<LLVMValueRef> *stores_to_target = new std::set<LLVMValueRef> ();
	
					// find all store instructions in R that write to the target address %t
					LLVMValueRef target_addr = LLVMGetOperand(instr,0);										
					LLVMValueRef value_stored = NULL;

									
					// go through R, if it writes to the same address at %t
					// add it to a set
					for (itr = R->begin(); itr != R->end(); itr++) {
						
						// we want store instructions in R that:
						//     write to the same address
						//     writes constants
						if (LLVMGetOperand(*itr,1) == target_addr) {
							stores_to_target->insert(*itr);	
							value_stored = LLVMGetOperand(*itr,0);			
						} 
					}
		
					// if we stored a NULl somehow?			
					if (value_stored == NULL) {continue;}
		
					// if it wasn't a constant
					if (!LLVMIsConstant(value_stored)) { continue;}


					// now we check if all values are the SAME constant
					for (itr = stores_to_target->begin(); itr != stores_to_target->end(); itr++) {
						if (LLVMGetOperand(*itr,0) != value_stored) 
							continue;
					}
			
					// replace all uses
					long long int_const = LLVMConstIntGetSExtValue(value_stored);
					LLVMReplaceAllUsesWith(instr, LLVMConstInt(LLVMInt32Type(),int_const,0) );
					changes_made++;
					instr_to_die->insert(instr);
					delete stores_to_target;
				
				}
			}

			// delete marked instructions	

			if (instr_to_die->size() > 0) {	
				for (itr = instr_to_die->begin(); itr != instr_to_die->end(); itr++) {
					LLVMInstructionEraseFromParent(*itr);
				}
			}
			if (had_to_create_new_R == 0) {delete R;}
			delete instr_to_die;	
		}
	}	
	return changes_made;
}



// CONSTANT PROPOGATION
int const_prop(LLVMModuleRef module) {
	int changes_made = 0;
//	printf("constant propogation!\n"); fflush(stdout);

// ========
// MAKE GEN (and a bit of prep for KILL)
// ========

	// big GEN unordered map
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_gen = new std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> ();
	
	// for KILL, the set of all store instructions
	std::set<LLVMValueRef> *all_stores = new std::set<LLVMValueRef> ();	

	make_gen(module,big_gen,all_stores);
	

// ========= 
// make KILL   (except I did step 1 of getting all store instructions earlier)
// =========	

	// big KILL unordered map
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_kill = new std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> ();

	make_kill(module,big_kill,all_stores);	
	
	delete all_stores;

// ===============
// make IN and OUT
// ===============


	// initialize big IN and big OUT
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_in = new std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> ();
	std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> *big_out = new std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>*> ();


	make_in_out(module, big_in, big_out, big_gen, big_kill);

// ===================================
// ACTUAL OPTIMIZATION WITH IN AND OUT
// ===================================

	changes_made = changes_made + actually_optimize(module,big_in);	
	

// real clean up hours!!!
	
	for (auto& it : *big_gen) {
		delete it.second;
     }	
	for (auto& it : *big_kill) {
		delete it.second;
     }	
	for (auto& it : *big_in) {
		delete it.second;

     }	
	for (auto& it : *big_out) {
		delete it.second;
     }	
	
	
	delete big_gen;
	delete big_kill;
	delete big_in;
	delete big_out;

	return changes_made;
}


/*

		Main Optimization Function
		(the one called in cmplr.c)

		
	Runs all the different optimization methods
	and keeps looping through them until no 
	more changes are being made.

*/
int optimize(char* filename) {
	assert(filename != NULL);

	LLVMModuleRef module;
	
	module = createLLVMModel(filename);	
	
	if (module == NULL) {
		fprintf(stderr,"module is NULL!\n");
		return -1;
	}
	
	int con_fold_changes, com_subexp_changes, dead_code_changes, con_prop_changes;
	
	int i = 1;
	do {
		printf("\n");
	
		con_prop_changes = const_prop(module);
//		printf("\tIteration %d: %d constant propogation changes\n",i,con_prop_changes);
	
		com_subexp_changes = com_subexp(module);
//		printf("\tIteration %d: %d common subexpression changes\n",i,com_subexp_changes);

		con_fold_changes = const_fold(module);
//		printf("\tIteration %d: %d constant folding changes.\n",i,con_fold_changes);
		
		dead_code_changes = dead_code(module);
//		printf("\tIteration %d: %d dead code changes\n",i,dead_code_changes);

		//		LLVMDumpModule(module);
		i++;	

	} while(con_prop_changes + con_fold_changes + dead_code_changes + com_subexp_changes != 0);
	
	char output_file[64];
	sprintf(output_file,"%s-faster",filename);

	LLVMPrintModuleToFile(module,output_file,NULL);

	LLVMDisposeModule(module);

	return 0;
}




