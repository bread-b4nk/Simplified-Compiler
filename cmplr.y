%{
#include <stdio.h>
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
extern int yylineno;
extern FILE *yyin;
int yyerror();


%}

%union{
	int ival;
	char* sname;
	
}


%token <sname> VAR
%token EXTERN RETURN PRINT READ INT VOID
%token WHILE IF ELSE 
%token <ival> ARITH // operators
%token <ival> NUM 
%token <sname> LEQ_OPTR GEQ_OPTR EQ_OPTR

%start program
//%start code_statements


%nonassoc THREE
%nonassoc TWO
// putting all the blocks less prioritized than code_statements above %nonassoc ONE
%nonassoc ONE


%%

program : externs func_block

externs : externs extern
	| extern

extern : EXTERN VOID PRINT '(' INT ')' ';' 
	| EXTERN INT READ '(' ')' ';' 

func_block : INT VAR '(' INT VAR ')' '{' code_block '}'
		| INT VAR '(' ')' '{' code_block '}'
		| VOID VAR '(' INT VAR ')' '{' code_block '}'
		| VOID VAR '(' ')' '{' code_block '}' 

code_block : var_decls code_statements
		| code_statements 

code_statements : code_statements code_statement  
		| code_statements if_else_block 
		| code_statements while_block 
		| if_else_block
		| while_block
		| code_statement 		

code_statement : assignment | return | kfunc_call 

if_else_block : if_statement '{' code_block '}' else_block
	| if_statement '{' code_block '}' 
	| if_statement code_block

else_block : ELSE code_statements | ELSE '{' code_block '}'

while_block : while_statement '{' code_block '}' | while_statement code_statements

if_statement : IF '(' bool_expression ')'  

while_statement :  WHILE '(' bool_expression ')'

arith_expression : add_expr | sub_expr | mul_expr | div_expr

add_expr : term '+' term
sub_expr : term '-' term
mul_expr : term '*' term
div_expr : term '/' term

bool_expression : lt_expr | gt_expr | leq_expr | geq_expr | eq_expr 

lt_expr : term '<' term
gt_expr : term '>' term 
leq_expr : term LEQ_OPTR term 
geq_expr : term GEQ_OPTR term
eq_expr : term EQ_OPTR term

term : VAR | NUM
	| '-' VAR
	| '-' NUM

var_decls : var_decls var_decl
	| var_decl

var_decl : INT VAR ';' 

kfunc_call : READ '(' ')' ';' %prec TWO | PRINT '(' VAR ')' ';' %prec TWO

assignment : VAR '=' arith_expression ';' | VAR '=' NUM ';' | VAR '=' VAR ';'
		| VAR '=' kfunc_call 

return : RETURN ';' | RETURN '(' arith_expression ')' ';' | RETURN arith_expression ';' | RETURN '(' NUM ')' ';' | RETURN NUM ';'

%%

int main(int argc, char** argv){
	if (argc == 2) {
		yyin = fopen(argv[1], "r");
	}

	yyparse();

	if (yyin != stdin) {
		fclose(yyin);
	}

	yylex_destroy();
	return 0;
} 

int yyerror(){
	fprintf(stderr, "Syntax error at line %d!\n",yylineno);
}
