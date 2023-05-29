#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "./ast/ast.h"
#include "./ast/smtic.h"
#include "optimizer.h"
#include "code_gen.h"
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
extern int code_gen(char* filename);

int main(int argc, char** argv){
	if (argc >=  2) {
		yyin = fopen(argv[1], "r");
	}	

	// call yyparse	
	if (yyparse() != 0) {
		fprintf(stderr,"yyparse failed!\n");
	}
	
//	printf("== Simplified Compiler ==\n");

	astNode *root = return_root();
	
	// printNode(root);
	// semantic analysis
	if (smtic_analyze(root) != 0){
		fprintf(stderr,"Semantic analysis failed!\n");
		exit(-1);
	}
//	printf("semantic analysis successful...\n");
		
	// generate llvm ir using clang
	char cmd[128];
	char* lastSlash = strrchr(argv[1],'/');
	char path[32];

	// if we're only given a C file, then we'll generate the llvm ourself
	// however, we won't be able to use external functions (print and read)
	if (argc == 2) {

		sprintf(cmd,"clang -S -emit-llvm %s -o llvm-ir.s\n",argv[1]);
		system(cmd);

	}

	else { // otherwise, we've been given an ll file as our second argument
		
		sprintf(cmd,"cp %s llvm-ir.s\n",argv[2]);
		system(cmd);
	}	
	
//	printf("generated llvm ir...\n");

	// check that file exists
	if (access("llvm-ir.s",0) != 0) {
		fprintf(stderr, "Generating llvm-ir failed!\n");
		exit(-1);
	}
	
//	printf("llvm generation successful...\n");	

	// optimize
	if (optimize("llvm-ir.s") != 0) {
		fprintf(stderr,"Optimization failed!\n");
		exit(-1);
	}

	if  (access("llvm-ir.s-faster",0) != 0)	{
		fprintf(stderr,"Optimization failed!\n");
		exit(-1);
	}

//	printf("optimization successful...\n");

	if (code_gen("llvm-ir.s-faster") != 0) {

		fprintf(stderr,"Code generation failed!\n");
		exit(-1);
	}
	

	//clean up
	if (yyin != stdin) {
		fclose(yyin);
	}

	yylex_destroy();
	freeProg(root);
	
	return 0;
} 


