%{
	#include <stdio.h>
	#include "y.tab.h"
	extern int yylex();
	extern int yylex_destroy();
	extern int yywrap();
	extern int yyerror();
	extern FILE *yyin;

	int yylineno; // should be handled in yybval later
%}


%%
int 		{printf("%s",yytext);return DATATYPE;}	// KEYWORDS: data types
void		{printf("%s",yytext);return DATATYPE;}
extern    	{printf("%s",yytext);return EXTERN;} 
print		{printf("%s",yytext);return KFUNC;}	// known functions
read		{printf("%s",yytext);return KFUNC;}
return		{printf("%s",yytext);return RETURN;}
if 		{printf("%s",yytext);return IF;}	// conditions
else		{printf("%s",yytext);return ELSE;}
while 		{printf("%s",yytext);return WHILE;}		// loops
\-?[0-9]+  	{printf("%s",yytext);return NUM;}	// integers
[a-zA-Z_][a-zA-Z0-9_]*       {printf("%s",yytext);return VAR;}// var name
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
"<="		{printf("%s",yytext);
		yylval.sname = strdup(yytext);return LEQ_OPTR;}
">="		{printf("%s",yytext);
		yylval.sname = strdup(yytext);return GEQ_OPTR;}
"=="		{printf("%s",yytext);
		yylval.sname = strdup(yytext);return EQ_OPTR;}
"\n"		{printf("%s",yytext);yylineno++;}
.|[ \t\n]       {printf("%s",yytext);}	
%%


int yywrap() {
	return 1;
}

