#ifndef SEM_ANAL
#define SEM_ANAL

#include<set>
#include<vector>

int smtic_analyze(astNode* root);
void checkNode(astNode *node, vector<set<char*>*>* sym_stack);
void checkStmt(astStmt *node, vector<set<char*>*>* sym_stack);
int findDeclaration(vector<set<char*>*>* sym_stack, char* name);

#endif
