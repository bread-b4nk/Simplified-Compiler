%{
#include <stdio.h>
#include "./ast/ast.h"
#include "./ast/smtic.h"
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
extern int yylineno;
extern FILE *yyin;
extern void yyerror(char* s);

astNode* return_root();
astNode* root;

%}

%union{
	int ival;
	char* sname;
	astNode* nptr;
	vector<astNode*> *svec_ptr;	
}


%token <sname> VAR
%token EXTERN RETURN PRINT READ INT VOID
%token WHILE IF ELSE ARITH 
%token <ival> NUM // operators
%token LEQ_OPTR GEQ_OPTR EQ_OPTR NOT_OPTR
		
%type <nptr> program extern_print extern_read func_block code_block
%type <nptr> code_statement if_else_block else_block
%type <nptr> while_block if_statement while_statement arith_expression bool_expression
%type <nptr> add_expr sub_expr mul_expr div_expr 
%type <nptr> lt_expr gt_expr geq_expr leq_expr eq_expr neq_expr term var_decl
%type <nptr> kfunc_call assignment return

%type <svec_ptr> var_decls code_statements

%start program


%nonassoc THREE
%nonassoc TWO
// putting all the blocks less prioritized than code_statements above %nonassoc ONE
%nonassoc ONE


%%

program : extern_print extern_read func_block {root = createProg($1,$2,$3);
						$$ = root;}
	| extern_read extern_print func_block {root = createProg($2,$1,$3);
						$$ = root;}

extern_print : EXTERN VOID PRINT '(' INT ')' ';' {$$ = createExtern("print");} 
extern_read :  EXTERN INT READ '(' ')' ';' 	 {$$ = createExtern("read");}

func_block : INT VAR '(' INT VAR ')' '{' code_block '}' {$$ = createFunc($2,createVar($5),$8);
							free($2);free($5);}
		| INT VAR '(' ')' '{' code_block '}'    {$$ = createFunc($2,NULL,$6);}
		| VOID VAR '(' INT VAR ')' '{' code_block '}' {$$ = createFunc($2,createVar($5), $8);
							free($2);free($5);}
		| VOID VAR '(' ')' '{' code_block '}' {$$ = createFunc($2,NULL,$6);
							free($2);}


// HEEERRREE MAKE SURE WE'RE FREEING CORRECTLY
code_block : var_decls code_statements {$1->insert($1->end(), $2->begin(), $2->end());
					$$ = createBlock($1);
					delete($2);}

/*				
	//for (std::vector<astNode*>::iterator it = $2->begin(); it != $2->end(); it++) {freeNode(*it);}
					
					printf("\n\n=============\n=\n");
					for (std::vector<astNode*>::iterator it = $2->begin(); it != $2->end(); it++) {printf("%p\n",*it);printNode(*it);}
					for (std::vector<astNode*>::iterator it = $1->begin(); it != $1->end(); it++) {printf("%p\n",*it);printNode(*it);}
					printf("\n=\n=============\n\n");
					}//$2->clear();}
				
*/

		| code_statements {$$ = createBlock($1);} 

// April 22nd: if I free $2, I get "Incorrect node type", if I don't free $2, I get leaks
//             I'm going to focus on bigger issues
code_statements : code_statements code_statement {$$ = $1;
						$$->push_back($2);}
		| code_statements if_else_block {$$ = $1;
						$$->push_back($2);}
		| code_statements while_block {$$ = $1;
						$$->push_back($2);}
		| if_else_block  {$$ = new vector<astNode*> ();
				$$->push_back($1);} 
		| while_block	 {$$ = new vector<astNode*> ();
				$$->push_back($1);}
		| code_statement {$$ = new vector<astNode*> ();
				$$->push_back($1);}	

code_statement : assignment   {$$ = $1;} 
		| return      {$$ = $1;}
		| kfunc_call  {$$ = $1;}

if_else_block : if_statement '{' code_block '}' else_block {$$ = createIf($1,$3,$5);}
	| if_statement '{' code_block '}' 	{$$ = createIf($1,$3);}
	| if_statement code_block               {$$ = createIf($1,$2);}

else_block : ELSE code_statements   		{$$ = createBlock($2);}
		| ELSE '{' code_block '}'       {$$ = $3;}

while_block : while_statement '{' code_block '}'  {$$ = createWhile($1,$3);}
		| while_statement code_statements {$$ = createWhile($1,createBlock($2));}

if_statement : IF '(' bool_expression ')'        {$$ = $3;}

while_statement :  WHILE '(' bool_expression ')' {$$ = $3;}

arith_expression : add_expr {$$ = $1;} 
		| sub_expr  {$$ = $1;}
		| mul_expr  {$$ = $1;}
		| div_expr  {$$ = $1;}

add_expr : term '+' term {$$ = createBExpr($1,$3,add);}
sub_expr : term '-' term {$$ = createBExpr($1,$3,sub);}
mul_expr : term '*' term {$$ = createBExpr($1,$3,mul);}
div_expr : term '/' term {$$ = createBExpr($1,$3,divide);}

bool_expression : lt_expr  {$$ = $1;}
	| gt_expr 	   {$$ = $1;}
	| leq_expr 	   {$$ = $1;}
	| geq_expr  	   {$$ = $1;}
	| eq_expr 	   {$$ = $1;}
	| neq_expr 	   {$$ = $1;}

lt_expr : term '<' term 	{$$ = createRExpr($1,$3,lt);}
gt_expr : term '>' term 	{$$ = createRExpr($1,$3,gt);}
leq_expr : term LEQ_OPTR term   {$$ = createRExpr($1,$3,le);}
geq_expr : term GEQ_OPTR term   {$$ = createRExpr($1,$3,ge);}
eq_expr : term EQ_OPTR term     {$$ = createRExpr($1,$3,eq);}
neq_expr : term NOT_OPTR term   {$$ = createRExpr($1,$3,neq);}

term : VAR 	   {$$ = createVar($1);
			free($1);}
	| NUM 	   {$$ = createCnst($1);}
	| '-' VAR  {$$ = createUExpr(createVar($2),uminus);
			free($2);} 
	| '-' NUM  {$$ = createUExpr(createCnst($2),uminus);}

var_decls : var_decls var_decl {$$ = $1;
				$$->push_back($2);}
	| var_decl {$$ = new vector<astNode*> ();
		   $$->push_back($1);}

var_decl : INT VAR ';' {$$ = createDecl($2);
				free($2);} 

kfunc_call : READ '(' ')' ';' %prec TWO {$$ = createCall("read");}
		| PRINT '(' VAR ')' ';' %prec TWO {$$ = createCall("print",createVar($3));
							free($3);}

assignment : VAR '=' arith_expression ';' {$$ = createAsgn(createVar($1),$3);
						free($1);}
		| VAR '=' NUM ';'       {$$ = createAsgn(createVar($1),createCnst($3));
						free($1);} 
		| VAR '=' VAR ';'       {$$ = createAsgn(createVar($1),createVar($3));
						free($1);free($3);}
		| VAR '=' kfunc_call 	{$$ = createAsgn(createVar($1),$3);
						free($1);}

// RETURN ';' 			      {$$ = createRet(NULL);}
return :  RETURN '(' arith_expression ')' ';' {$$ = createRet($3);} 
	| RETURN arith_expression ';'         {$$ = createRet($2);}
	| RETURN '(' NUM ')' ';'              {$$ = createRet(createCnst($3));}
	| RETURN NUM ';'		      {$$ = createRet(createCnst($2));}
	| RETURN VAR ';'					{$$ = createRet(createVar($2));
											free($2);}
	| RETURN '(' VAR ')' ';'			{$$ = createRet(createVar($3));
											free($3);}

%%

astNode* return_root() {
	return root; 
}

void yyerror(char *s){
	fprintf(stderr, "Syntax error at line %d!\n",yylineno);
	exit(1);
}

