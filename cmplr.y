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


%token <sname> DATATYPE
%token EXTERN RETURN KFUNC
%token WHILE IF ELSE 
%token <ival> ARITH // operators
%token <ival> NUM VAR //? IS VAR AN INT OR A STRING
%token <sname> LEQ_OPTR GEQ_OPTR EQ_OPTR

%start code_statements


%nonassoc THREE
%nonassoc TWO
%nonassoc NONPRIO
%nonassoc PRIO_SHIFT
// putting all the blocks less prioritized than code_statements above %nonassoc ONE
%nonassoc ONE


%%

//code

//code_block :
// code_statements_ends_with_func_block : func_block
			//	| code_statements func_block
			//	| code_statements_ends_with_func_block func_block

code_statements : code_statements code_statement 
		| code_statements if_else_block 
		| code_statements while_block 
		| code_statements func_block 
		| func_block
		| if_else_block
		| while_block
		| code_statement 		

code_statement : assignment | var_declaration | return | kfunc_call | func_declaration | kfunc_declaration

if_else_block : if_statement '{' code_statements '}' else_block
	| if_statement '{' code_statements '}' 
	| if_statement code_statements

else_block : ELSE code_statements | ELSE '{' code_statements '}'

while_block : while_statement '{' code_statements '}' | while_statement code_statements


func_block : types VAR '(' DATATYPE VAR ')' '{' code_statements '}'
		| types VAR '(' ')' '{' code_statements '}'

types : EXTERN types // could be types EXTERN?
	| types DATATYPE
	| DATATYPE

if_statement : IF '(' bool_expression ')'  


while_statement :  WHILE '(' bool_expression  ')'


arith_expression : add_expr | sub_expr | mul_expr | div_expr

add_expr : VAR '+' VAR | VAR '+' NUM | NUM '+' NUM | NUM '+' VAR
sub_expr : VAR '-' VAR | VAR '-' NUM | NUM '-' NUM | NUM '-' VAR
mul_expr : VAR '*' VAR | VAR '*' NUM | NUM '*' NUM | NUM '*' VAR
div_expr : VAR '/' VAR | VAR '/' NUM | NUM '/' NUM | NUM '/' VAR

bool_expression : lt_expr | gt_expr | leq_expr | geq_expr | eq_expr 

lt_expr : VAR '<' VAR | VAR '<' NUM | NUM '<' NUM | NUM '<' VAR
gt_expr : VAR '>' VAR | VAR '>' NUM | NUM '>' NUM | NUM '>' VAR
leq_expr : VAR LEQ_OPTR VAR | VAR LEQ_OPTR NUM | NUM LEQ_OPTR NUM | NUM LEQ_OPTR VAR 
geq_expr : VAR GEQ_OPTR VAR | VAR GEQ_OPTR NUM | NUM GEQ_OPTR NUM | NUM GEQ_OPTR VAR
eq_expr : VAR EQ_OPTR VAR | VAR EQ_OPTR NUM | NUM EQ_OPTR NUM | NUM EQ_OPTR VAR


var_declaration : types VAR ';' 

func_declaration : types VAR '(' ')' ';' %prec ONE
		| types VAR '(' types VAR ')' ';' %prec ONE

kfunc_declaration : types KFUNC '(' ')' ';' %prec ONE
		| types KFUNC '(' types ')' ';' %prec ONE

kfunc_call : KFUNC '(' ')' ';' %prec TWO | KFUNC '(' VAR ')' ';' %prec TWO

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
