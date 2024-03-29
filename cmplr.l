%{
	#include <stdio.h>
	#include "./ast/ast.h"
	#include "y.tab.h"
	#include <string.h>
	extern int yylex();
	extern int yylex_destroy();
//	extern int yywrap();
	extern int yyerror();
	extern FILE *yyin;


%}


%%
int 		{printf("%s",yytext);return INT;}	// KEYWORDS: data types
void		{printf("%s",yytext);return VOID;}
extern    	{printf("%s",yytext);return EXTERN;} 
print		{printf("%s",yytext);return PRINT;}	// known functions
read		{printf("%s",yytext);return READ;}
return		{printf("%s",yytext);return RETURN;}
if 		{printf("%s",yytext);return IF;}	// conditions
else		{printf("%s",yytext);return ELSE;}
while 		{printf("%s",yytext);return WHILE;}		// loops
\-?[0-9]+  	{printf("%s",yytext);yylval.ival = atoi(yytext); 
				return NUM;}	// integers
[a-zA-Z_][a-zA-Z0-9_]*       {printf("%s",yytext);yylval.sname =  strdup(yytext);
				return VAR;}// var name
";"		{printf("%s",yytext);return yytext[0];}
"("		{printf("%s",yytext);return yytext[0];}
")"		{printf("%s",yytext);return yytext[0];}
"{"		{printf("%s",yytext);return yytext[0];}
"}"		{printf("%s",yytext);return yytext[0];}
"="		{printf("%s",yytext);return yytext[0];}
"+"		{printf("%s",yytext);return yytext[0];}
"-"		{printf("%s",yytext);return yytext[0];}
"*"		{printf("%s",yytext);return yytext[0];}
"/"		{printf("%s",yytext);return yytext[0];}
"<"		{printf("%s",yytext);return yytext[0];}
">"		{printf("%s",yytext);return yytext[0];}
"<="		{printf("%s",yytext);return LEQ_OPTR;}
">="		{printf("%s",yytext);return GEQ_OPTR;}
"=="		{printf("%s",yytext);return EQ_OPTR;}
"!="		{printf("%s",yytext);return NOT_OPTR;}
"\n"		{printf("%s",yytext);yylineno++;}
.|[ \t]       {printf("%s",yytext);}	
%%


int yywrap() {
	return 1;
}

