#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "./ast/ast.h"
#include "./ast/smtic.h"
#include "optimizer.h"
#define prt(x) if(x) { printf("%s\n", x); }

extern int yylex();
extern int yywrap();
extern int yylineno;
extern int yyparse();
extern int yylex_destroy();
extern FILE *yyin;
extern astNode* return_root();

void yyerror(char* s);

extern int optimize(char* filename);

int main(int argc, char** argv){
	if (argc == 2) {
		yyin = fopen(argv[1], "r");
	}	

	// call yyparse	
	if (yyparse() != 0) {
		fprintf(stderr,"yyparse failed!\n");
	}
	
	printf("== Simplified Compiler ==\n");

	astNode *root = return_root();
	
	// printNode(root);
	// semantic analysis
	if (smtic_analyze(root) != 0){
		fprintf(stderr,"Semantic analysis failed!\n");
		exit(-1);
	}
	printf("semantic analysis successful...\n");
	
	// generate llvm ir using clang
	char cmd[64];
	sprintf(cmd,"clang -S -emit-llvm %s -o llvm-ir.s\n",argv[1]);
	system(cmd);

	printf("generated llvm ir...\n");
	// check that file exists
	if (access("llvm-ir.s",0) != 0) {
		fprintf(stderr, "Generating llvm-ir failed!\n");
		exit(-1);
	}
	
	printf("llvm generation successful...\n");	

	// optimize
	if (optimize("llvm-ir.s") != 0) {
		fprintf(stderr,"Optimization failed!\n");
		exit(-1);
	}
	printf("optimization successful...\n");
	//clean up
	if (yyin != stdin) {
		fclose(yyin);
	}

	yylex_destroy();
	freeProg(root);
	
	return 0;
} 

