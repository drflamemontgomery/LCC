%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "tree.h"

void yyerror(void *lloc, const char*msg);
int yylex (void *, void *);

extern FILE* yyin;

struct tree *head = NULL;
%}

%locations
%define api.pure
%define parse.error detailed
%glr-parser


%union {
  int ival;
  double fval;
  char cval;
  char* symbol;
  struct tree *ast;
  struct type_id *tid;
}

%token <ival> INTEGER "integer"
%token <fval> FLOAT "float"
%token <cval> CHAR "char"
%token <symbol> SYMBOL "symbol"
%token <symbol> STRING "string"

%token CONST VOLATILE RESTRICT ATOMIC
%token IF WHILE DOWHILE CASE COND FOR LET
%token DEFUN DEFMETHOD DEFGENERIC DEFVAR
%token T NIL
%token INCLUDE
%token ADDR DEREF
%token LT GT LE GE AND OR NOT

%type <ast> exp condition body body_ call

%type <ast> include
%type <ast> defvar fndecl arglist arglist_fields
%type <ast> let_stmt let_arg_list
%type <ast> control_stmt while_stmt do_while_stmt for_stmt for_assign_body if_stmt
%type <ast> cond_stmt cond_body case_stmt case_body

%type <ast> type
%type <symbol> typename
%type <tid> modified_typename
%%

input:
      %empty
| input defvar { head = append_tree(head, $2); }
| input fndecl { head = append_tree(head, $2); }
| input include { head = append_tree(head, $2); }
;

exp:
   INTEGER { $$ = build_int_cst($1); }
| SYMBOL { $$ = build_var_ref($1); }
| STRING { $$ = build_string_cst($1); }
| CHAR   { $$ = build_char_cst($1); }
| T { $$ = build_bool_cst(true); }
| NIL { $$ = build_bool_cst(false); }
| call { $$ = $1; }
| defvar { $$ = $1; }
;
condition: exp { $$ = $1; };

call:
  '(' SYMBOL body ')' { $$ = build_fn_call($2, $3); }
| '(' '+' body ')' { $$ = build_binop('+', $3); }
| '(' '-' body ')' { $$ = build_binop('-', $3); }
| '(' '*' body ')' { $$ = build_binop('*', $3); }
| '(' '/' body ')' { $$ = build_binop('/', $3); }
| '(' '=' exp exp ')' { $$ = build_compare(OP_EQL, $3, $4); }
| '(' LT exp exp ')' { $$ = build_compare(OP_LT, $3, $4); }
| '(' GT exp exp ')' { $$ = build_compare(OP_GT, $3, $4); }
| '(' LE exp exp ')' { $$ = build_compare(OP_LE, $3, $4); }
| '(' GE exp exp ')' { $$ = build_compare(OP_GE, $3, $4); }
| '(' AND exp exp ')' { $$ = build_compare(OP_AND, $3, $4); }
| '(' OR exp exp ')' { $$ = build_compare(OP_OR, $3, $4); }
| '(' NOT exp ')' { $$ = build_compare(OP_NOT, $3, NULL); }
| '(' DEREF exp ')' { $$ = build_deref($3); }
| '(' ADDR exp ')' { $$ = build_addr($3); }
;

fndecl: '(' DEFUN SYMBOL ':' type arglist body ')' { $$ = build_fn($3, $5, $6, $7); } ;
defvar:
  '(' DEFVAR SYMBOL ':' type exp ')' { $$ = build_var(VAR_DECL, $3, $5, $6); }
| '(' DEFVAR SYMBOL ':' type ')' { $$ = build_var(VAR_DECL, $3, $5, NULL); }
;

include: '(' INCLUDE STRING ')' { $$ = build_include($3); };


body: body_ { $$ = reverse_tree($1); }
body_:
    %empty { $$ = NULL; }
| body_ exp { $$ = append_tree($1, $2); }
| body_ control_stmt { $$ = append_tree($1, $2); }
| body_ let_stmt { $$ = append_tree($1, $2); }
;

arglist:
       '(' arglist_fields ')' { $$ = reverse_tree($2); }
;

arglist_fields:
              %empty { $$ = NULL; }
| arglist_fields '(' SYMBOL ':' type ')' { $$ = append_tree($1, build_var(PARM_DECL, $3, $5, NULL)); }
;

type: modified_typename { $$ = build_type_expr($1); }
| '*' type { $$ = $2; add_type_ptr($$, SINGLE_PTR, 0); }
| '[' ']' type { $$ = $3; add_type_ptr($$, MULTI_PTR, 0);}
| '[' INTEGER ']' type { $$ = $4; add_type_ptr($$, SIZED_PTR, $2); }
;

typename: SYMBOL { $$ = $1; }
modified_typename: typename { $$ = build_tid($1, MOD_NONE); }
| CONST typename { $$ = build_tid($2, MOD_CONST); }
| VOLATILE typename { $$ = build_tid($2, MOD_VOLATILE); }
| RESTRICT typename { $$ = build_tid($2, MOD_RESTRICT); }
| ATOMIC typename { $$ = build_tid($2, MOD_ATOMIC); }

if_stmt:
       '(' IF condition exp ')' { $$ = build_if_else_stmt($3, $4, NULL); }
| '(' IF condition exp exp ')' { $$ = build_if_else_stmt($3, $4, $5); }
;

cond_stmt: '(' COND '(' cond_body ')' ')' { $$ = build_cond_stmt(reverse_tree($4)); };
cond_body:
         '(' condition body ')' { $$ = build_cond_body($2, $3); }
| cond_body '(' condition body ')' { $$ = append_tree($1, build_cond_body($3, $4)); }
;

case_stmt: '(' CASE exp '(' case_body ')' ')' { $$ = build_case_stmt($3, $5); };
case_body:
         '(' exp body ')' { $$ = build_case_body($2, $3); }
| case_body '(' exp body ')' { $$ = append_tree($1, build_case_body($3, $4)); }
;


while_stmt: '(' WHILE condition body ')' {$$ = build_while_stmt(WHILE_STMT, $3, $4);};

do_while_stmt: '(' DOWHILE condition body ')' {$$ = build_while_stmt(DOWHILE_STMT, $3, $4);};

for_stmt: '(' FOR ':' type '(' for_assign_body ')' condition exp body ')' { $$ = build_for_stmt($4, $6, $8, $9, $10);};
for_assign_body:
               %empty { $$ = NULL; }
| for_assign_body '(' SYMBOL exp ')' { $$ = append_tree($1, build_set_expr($3, $4));}
;

control_stmt: if_stmt {$$ = $1;} 
| cond_stmt{$$ = $1;}
| case_stmt {$$ = $1;}
| while_stmt {$$ = $1;}
| do_while_stmt {$$ = $1;}
| for_stmt{$$ = $1;};

let_stmt: '(' LET '(' let_arg_list ')' body ')' {$$ = build_let_stmt(reverse_tree($4), $6);};
let_arg_list:
            '(' SYMBOL ':' type exp ')' { $$ = build_var(VAR_DECL, $2, $4, $5);  }
| let_arg_list '(' SYMBOL ':' type  exp ')' { $$ = append_tree($1, build_var(VAR_DECL, $3, $5, $6)); }
;


%%

const char* current_file = "stdin";

void yyerror (void *locp, char const * err) {
YYLTYPE* llocp = locp;
  errorat("%s", current_file, llocp->first_line, llocp->first_column, err);
}


int main(int argc, char *const argv[]) {
  argc--;
  argv++;
  if(argc > 0) {
    current_file = argv[0];
    printf("Open %s\n", argv[0]);
    yyin = fopen(argv[0], "r");
  }
  else yyin = stdin;
  int ret = yyparse();

  head = reverse_tree(head);
  print_tree(head);
  destroy_tree(head);
  return ret;
}
