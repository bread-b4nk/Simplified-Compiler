#include "ast.h"
#include "smtic.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<set>
#include<vector>


/*  checks if every variable is declared before it is used
	inputs: 	
		astNode* root   -   root of ast
	outputs:
		0   -   if success
		1   -   if found use without declaration
*/
int smtic_analyze(astNode* root) {
	assert(root != NULL);

	// printNode(root);
	
	// stack of symbol tables	
	vector<set<char*>*>* sym_stack = new vector<set<char*>*> ();
	

	// tree traversal
	if (checkNode(root,sym_stack) != 0) {
		return 1;
	}
	
	// cleanup
	while (sym_stack->size() != 0) {
		freeTable(sym_stack->back());
		sym_stack->pop_back();
	}
	delete(sym_stack);
	
	return 0;
}

int checkNode(astNode *node, vector<set<char*>*>* sym_stack){
	assert(node != NULL);
	
	switch(node->type){
		case ast_prog:{
						checkNode(node->prog.func,sym_stack);	
						break;
					  }
		case ast_func:{
						
						// create new symbol table to hold parameters						
						set<char*> *sym_table = new set<char*>();
  
						if (node->func.param != NULL) {
							
							// copy parameter variable name into symbol table
							char* name = node->func.param->var.name;

							char* param_name = (char *) calloc(1, sizeof(char) * (strlen(name)+1));
							strcpy(param_name,name);

							sym_table->insert(param_name);							
							
							sym_stack->push_back(sym_table);	
							// not gna check the func param then haha
							// checkNode(node->func.param, sym_stack);
						}
						// recurse onto children
						checkNode(node->func.body, sym_stack);
						
						// spring cleaning
						freeTable(sym_stack->back());
						sym_stack->pop_back();
						break;
					  }
		case ast_stmt:{ 
						// we'll check for declaration in checkStmt
						astStmt stmt= node->stmt;
						checkStmt(&stmt, sym_stack);
						break;
					  }
		case ast_extern:{
						// lol get ignored
						break;
					  }
		case ast_var: {
						// if we can't find a declaration from earlier, we're not good
						if (findDeclaration(sym_stack,node->var.name) != 0) {
								fprintf(stderr,"Unable to find declaration for %s\n",node->var.name);
								return 1;
						}
						break;

					  }
		case ast_cnst: {
						// shouldn't be here						
						 break;
					  }
		case ast_rexpr: {
						// need to recurse into lhs and rhs before checking
						// lhs and rhs are astNode's that are variables or constants
						checkNode(node->rexpr.lhs, sym_stack);
						checkNode(node->rexpr.rhs, sym_stack);
						break;
					  }
		case ast_bexpr: {
						// need to recurse into lhs and rhs before checking
						checkNode(node->bexpr.lhs, sym_stack);
						checkNode(node->bexpr.rhs, sym_stack);
						break;
					  }
		case ast_uexpr: {
						// need to recurse into lhs and rhs before checking
						checkNode(node->uexpr.expr, sym_stack);
						break;
					  }
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	return 1;
				 }
	}
	return 0;
}

int checkStmt(astStmt *stmt, vector<set<char*>*> *sym_stack){
	assert(stmt != NULL);

	switch(stmt->type){
		case ast_call: { 
							if (stmt->call.param != NULL){
								// we're using some variable as a param, so we must check it
								checkNode(stmt->call.param, sym_stack);
							}
							break;
						}
		case ast_ret: {
							checkNode(stmt->ret.expr, sym_stack);
							break;
						}
		case ast_block: {
							
							// create new and empty symbol table
							set<char*> *sym_table = new set<char*>();
							sym_stack->push_back(sym_table);
							
							// call checkNode on all statements in block
							vector<astNode*> slist = *(stmt->block.stmt_list);
							vector<astNode*>::iterator it = slist.begin();
							while (it != slist.end()){
								checkNode(*it, sym_stack);
								it++;
							}
							
							// POP SYMBOL TABLE
							freeTable(sym_stack->back());
							sym_stack->pop_back();
							break;
						}
		case ast_while: {
							checkNode(stmt->whilen.cond, sym_stack);
							checkNode(stmt->whilen.body, sym_stack);
							break;
						}
		case ast_if: {
							checkNode(stmt->ifn.cond, sym_stack);
							checkNode(stmt->ifn.if_body, sym_stack);
							if (stmt->ifn.else_body != NULL)
							{
								checkNode(stmt->ifn.else_body, sym_stack);
							}
							break;
						}
		case ast_asgn:	{
							checkNode(stmt->asgn.lhs, sym_stack);
							checkNode(stmt->asgn.rhs, sym_stack);
							break;
						}
		case ast_decl:	{
							
							// copy var name
							char* name = (char *) calloc(1, sizeof(char) * (strlen(stmt->decl.name)+1));
							strcpy(name,stmt->decl.name);
							
							// add var name to current sym table
							set<char*> *sym_table = sym_stack->back();
							sym_table->insert(name);
								
							break;
						}
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	return 1;
				 }
	}
	return 0;
}

/*  looks through our stack of symbol tables for a declaration of a specific variable name
		
		inputs:    
			sym_stack   -   stack of symbol tables
			name		- 	name of variable we're looking for

		returns:
			0			-	exists
			1			- 	does not
*/


void freeTable(set<char*>* sym_table) {
	assert(sym_table != NULL);
	
	set<char*>::iterator it = sym_table->begin();
	while(it != sym_table->end()) {
		free(*it);
		it++;
	}
	delete(sym_table);
 

}

int findDeclaration(vector<set<char*>*> *sym_stack, char* name) {
	//printf("Checking declaration of %s\n",name);
	assert(sym_stack != NULL && name != NULL);	

	vector<set<char*>*>::iterator it = sym_stack->begin();
	while (it != sym_stack->end()) {

		set<char*>::iterator it1 = (*it)->begin();
		while(it1 != (*it)->end()) {
			if (strcmp(name,*it1) == 0) {return 0;} 
			it1++;		
		}

		it++;
	} 	
	
	return 1;
}


