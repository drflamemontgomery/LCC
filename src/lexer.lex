%{
  #include "lang.h"
  #include <math.h>
  #include <string.h>
#include <stdio.h>

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;

#define MOVECOL(n) yylloc->first_column += n;
%}

COMMENT ;.*\n
WHITESPACE [ \t\r]
SPECIAL ['\(\)"]
STRING_LITERAL \"([^\\"]|\\.|\\\n)*\"
CHAR_LITERAL \'[^\\]\'
HEX_CHAR \'\\x[0-9a-fA-F]{1,2}\'
OCTAL_CHAR \'\\[0-7]{1,3}\'

%option noyywrap
%option bison-locations
%option yylineno

%%

{COMMENT}      {MOVECOL(yyleng)}// eat
\n* {yylloc->first_column = 0;}
{WHITESPACE}*  {MOVECOL(yyleng)}// ignore

{STRING_LITERAL} {MOVECOL(yyleng);yylval->symbol = strdup(yytext); return STRING;}
{CHAR_LITERAL} {MOVECOL(yyleng);yylval->cval = yytext[1]; return CHAR;}
{HEX_CHAR} {MOVECOL(yyleng);yylval->cval = strtol(&yytext[3], NULL, 16); return CHAR;}
{OCTAL_CHAR} {MOVECOL(yyleng);yylval->cval = strtol(&yytext[2], NULL, 8); return CHAR;}

[0-9]+\.[0-9]* {MOVECOL(yyleng); yylval->fval = atof(yytext); return FLOAT; }
0[0-7]+ {MOVECOL(yyleng); yylval->ival = strtol(&yytext[2], NULL, 8); return INTEGER; }
[0-9]+ { MOVECOL(yyleng); yylval->ival = atoi(yytext); return INTEGER; }
0x[0-9a-fA-F]+ {MOVECOL(yyleng); yylval->ival = strtol(&yytext[2], NULL, 16); return INTEGER; }
0b[01]+ {MOVECOL(yyleng); yylval->ival = strtol(&yytext[2], NULL, 2); return INTEGER; }
defun           {MOVECOL(yyleng);return DEFUN;}
defgeneric      {MOVECOL(yyleng);return DEFGENERIC;}
defmethod       {MOVECOL(yyleng);return DEFMETHOD;}
defvar          {MOVECOL(yyleng);return DEFVAR;}

const           {MOVECOL(yyleng);return CONST;}
volatile           {MOVECOL(yyleng);return VOLATILE;}
restrict           {MOVECOL(yyleng);return RESTRICT;}
atomic           {MOVECOL(yyleng);return ATOMIC;}

include {MOVECOL(yyleng);return INCLUDE;}

if {MOVECOL(yyleng);return IF;}
while {MOVECOL(yyleng);return WHILE;}
do-while {MOVECOL(yyleng);return DOWHILE;}
case {MOVECOL(yyleng);return CASE;}
cond {MOVECOL(yyleng);return COND;}
for {MOVECOL(yyleng); return FOR;}
let {MOVECOL(yyleng); return LET;}

addr {MOVECOL(yyleng); return ADDR;}
deref {MOVECOL(yyleng); return DEREF;}

t {MOVECOL(yyleng);return T; }
nil {MOVECOL(yyleng);return NIL; }

"<" {MOVECOL(yyleng);return LT; }
">" {MOVECOL(yyleng);return GT; }
"<=" {MOVECOL(yyleng);return LE; }
">=" {MOVECOL(yyleng);return GE; }
and {MOVECOL(yyleng);return AND; }
or {MOVECOL(yyleng);return OR; }
not {MOVECOL(yyleng);return NOT; }

[_a-zA-Z\$][\-a-zA-Z0-9\$]* {MOVECOL(yyleng); yylval->symbol = strdup(yytext); return SYMBOL; }
. {MOVECOL(yyleng); return yytext[0]; }
