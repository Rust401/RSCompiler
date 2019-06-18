%{
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <memory.h>
#include "ASTNodes.h"
#include "grammar.hpp"
#define SAVE_TOKEN yylval.string = new string(yytext)
#define TOKEN(t) ( yylval.token = t)

//if flag==0 then the token will not output
//if flag==1 then the token will be displayed when parsing is processing
int flag=0;

static FILE* yyparse_file_ptr;
%}

%option noyywrap

%%
"#".*                   ;
[ \t\r\n]				;
"int"                   SAVE_TOKEN; if(flag==1)puts("TYINT");  return TYINT;
"double"                SAVE_TOKEN; if(flag==1)puts("TYDOUBLE"); return TYDOUBLE;
"float"                 SAVE_TOKEN; if(flag==1)puts("TYFLOAT"); return TYFLOAT;
"char"                  SAVE_TOKEN; if(flag==1)puts("TYCHAR"); return TYCHAR;
"bool"                  SAVE_TOKEN; if(flag==1)puts("TYBOOL"); return TYBOOL;
"string"                SAVE_TOKEN; if(flag==1)puts("TYSTRING"); return TYSTRING;
"void"                  SAVE_TOKEN; if(flag==1)puts("TYVOID"); return TYVOID;
"extern"                SAVE_TOKEN; if(flag==1)puts("TEXTERN"); return TEXTERN;
"if"                    if(flag==1)puts("TIF"); return TOKEN(TIF);
"else"                  if(flag==1)puts("TELSE"); return TOKEN(TELSE);
"return"                if(flag==1)puts("TRETURN"); return TOKEN(TRETURN);
"for"                   if(flag==1)puts("TFOR"); return TOKEN(TFOR);
"while"                 if(flag==1)puts("TWHILE"); return TOKEN(TWHILE);
"struct"                if(flag==1)puts("TSTRUCT"); return TOKEN(TSTRUCT);
[a-zA-Z_][a-zA-Z0-9_]*	SAVE_TOKEN; if(flag==1)puts("TIDENTIFIER"); return TIDENTIFIER;
[0-9]+\.[0-9]*			SAVE_TOKEN; if(flag==1)puts("TDOUBLE"); return TDOUBLE;
[0-9]+  				SAVE_TOKEN; if(flag==1)puts("TINTEGER"); return TINTEGER;
\"(\\.|[^"])*\"         SAVE_TOKEN; if(flag==1)puts("TLITERAL"); return TLITERAL;
"="						if(flag==1)puts("TEQUAL"); return TOKEN(TEQUAL);
"=="					if(flag==1)puts("TCEQ"); return TOKEN(TCEQ);
"!="                    if(flag==1)puts("TCNE"); return TOKEN(TCNE);
"<"                     if(flag==1)puts("TCLT"); return TOKEN(TCLT);
"<="                    if(flag==1)puts("TCLE"); return TOKEN(TCLE);
">"                     if(flag==1)puts("TCGT"); return TOKEN(TCGT);
">="                    if(flag==1)puts("TCGE"); return TOKEN(TCGE);
"("                     if(flag==1)puts("TLPAREN"); return TOKEN(TLPAREN);
")"                     if(flag==1)puts("TRPAREN"); return TOKEN(TRPAREN);
"{"                     if(flag==1)puts("TLBRACE"); return TOKEN(TLBRACE);
"}"                     if(flag==1)puts("TRBRACE"); return TOKEN(TRBRACE);
"["                     if(flag==1)puts("TLBRACKET"); return TOKEN(TLBRACKET);
"]"                     if(flag==1)puts("TRBRACKET"); return TOKEN(TRBRACKET);
"."                     if(flag==1)puts("TDOT"); return TOKEN(TDOT);
","                     if(flag==1)puts("TCOMMA"); return TOKEN(TCOMMA);
"+"                     if(flag==1)puts("TPLUS"); return TOKEN(TPLUS);
"-"                     if(flag==1)puts("TMINUS"); return TOKEN(TMINUS);
"*"                     if(flag==1)puts("TMUL"); return TOKEN(TMUL);
"/"                     if(flag==1)puts("TDIV"); return TOKEN(TDIV);
"&"                     if(flag==1)puts("TAND"); return TOKEN(TAND);
"|"                     if(flag==1)puts("TOR"); return TOKEN(TOR);
"^"                     if(flag==1)puts("TXOR"); return TOKEN(TXOR);
"%"                     if(flag==1)puts("TMOD"); return TOKEN(TMOD);
">>"                    if(flag==1)puts("TSHIFTR"); return TOKEN(TSHIFTR);
"<<"                    if(flag==1)puts("TSHIFTL"); return TOKEN(TSHIFTL);
";"                     if(flag==1)puts("TSEMICOLON"); return TOKEN(TSEMICOLON);
.						printf("Unknown token:%s\n", yytext); yyterminate();

%%

