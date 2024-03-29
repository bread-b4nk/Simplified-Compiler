#ifndef SEM_ANAL
#define SEM_ANAL

#include<set>
#include<vector>

int smtic_analyze(astNode* root);
int checkNode(astNode *node, vector<set<char*>*>* sym_stack);
int checkStmt(astStmt *node, vector<set<char*>*>* sym_stack);
int findDeclaration(vector<set<char*>*>* sym_stack, char* name);
void freeTable(set<char*>* sym_table);
#endif
