%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "tree.h"

void lccerror(void *lloc, const char*msg);
int lcclex (void *, void *);
int c_main (char *const);

extern FILE* lccin;

struct tree *head = NULL;
#define NOTYPE build_type_expr((struct location){0, 0, 0, 0}, build_tid(NULL, MOD_MONOMORPH))

int n_errors = 0;
%}

%locations
%define api.pure
%define api.prefix {lcc}
%define api.location.type {struct location}
%define parse.error detailed
%code requires { #include "tree.h" }
%glr-parser

%union {
  int ival;
  double fval;
  char cval;
  char* symbol;
  struct tree *ast;
  struct type_id *tid;
}

%destructor { free($$); } <symbol>
%destructor { destroy_tree($$); } <ast>
%destructor { destroy_tid($$); } <tid>

%token <ival> INTEGER "integer"
%token <fval> FLOAT "float"
%token <cval> CHAR "char"
%token <symbol> SYMBOL "symbol"
%token <symbol> STRING "string"

%token CONST VOLATILE RESTRICT ATOMIC
%token IF WHILE DOWHILE CASE COND FOR LET
%token DEFUN DEFMETHOD DEFGENERIC DEFVAR
%token DECLARE DECLAIM PROCLAIM TYPE
%token T NIL
%token INCLUDE
%token ADDR AREF INC DEC CAST
%token LT GT LE GE AND OR NOT
%token OPTIONAL KEY REST AUX

%token I8 I16 I32 I64 I128
%token U8 U16 U32 U64 U128
%token F32 F64 F128
%token C32 C64 C128

%type <ast> exp condition body body_expr call

%type <ast> include
%type <ast> defvar fndecl //arglist arglist_fields
%type <ast> let_stmt let_arg_list
%type <ast> control_stmt while_stmt do_while_stmt for_stmt for_assign_body if_stmt
%type <ast> cond_stmt cond_body case_stmt case_body
%type <ast> lambda_list lambda_rest_arg lambda_regular_args lambda_key_args lambda_aux_args lambda_optional_args
%type <ast> lambda_optional_body lambda_key_body lambda_aux_body

%type <ast> declare_expr declaim_expr declarations symbol_list string_list
%type <ast> type
%type <symbol> typename
%type <tid> modified_typename
%%

input:
      %empty
| defvar input { head = append_tree(head, $1); }
| fndecl input { head = append_tree(head, $1); }
| include input { head = append_tree(head, $1); }
| declaim_expr input { head = append_tree(head, $1); }
;

exp:
   INTEGER { $$ = build_int_cst(@1, $1); }
| SYMBOL { $$ = build_var_ref(@1, $1); }
| STRING { $$ = build_string_cst(@1, $1); }
| CHAR   { $$ = build_char_cst(@1, $1); }
| T { $$ = build_bool_cst(@1, true); }
| NIL { $$ = build_bool_cst(@1, false); }
| call { $$ = $1; }
| defvar { $$ = $1; }
;
condition: exp { $$ = $1; };

call:
  '(' SYMBOL body ')' { $$ = build_fn_call(@1, $2, $3); }
| '(' '+' body ')' { $$ = build_binop(@1, '+', $3); }
| '(' '-' body ')' { $$ = build_binop(@1, '-', $3); }
| '(' '*' body ')' { $$ = build_binop(@1, '*', $3); }
| '(' '/' body ')' { $$ = build_binop(@1, '/', $3); }
| '(' '=' exp exp ')' { $$ = build_compare(@1, OP_EQL, $3, $4); }
| '(' LT exp exp ')' { $$ = build_compare(@1, OP_LT, $3, $4); }
| '(' GT exp exp ')' { $$ = build_compare(@1, OP_GT, $3, $4); }
| '(' LE exp exp ')' { $$ = build_compare(@1, OP_LE, $3, $4); }
| '(' GE exp exp ')' { $$ = build_compare(@1, OP_GE, $3, $4); }
| '(' AND exp exp ')' { $$ = build_compare(@1, OP_AND, $3, $4); }
| '(' OR exp exp ')' { $$ = build_compare(@1, OP_OR, $3, $4); }
| '(' NOT exp ')' { $$ = build_compare(@1, OP_NOT, $3, NULL); }
| '(' AREF exp body ')' { $$ = build_aref(@1, $3, $4); }
| '(' ADDR exp ')' { $$ = build_addr(@1, $3); }
| '(' INC exp ')' { $$ = build_inc(@1, $3); }
| '(' DEC exp ')' { $$ = build_dec(@1, $3); }
| '(' CAST type exp ')' { $$ = build_cast(@1, $3, $4); }
| declaim_expr { $$ = $1; }
;

declaim_expr: '(' DECLAIM declarations ')' { $$ = $3; };

fndecl:
  '(' DEFUN SYMBOL lambda_list body ')' { $$ = build_fn(@1, $3, NOTYPE, $4, $5); }
;
defvar:
  '(' DEFVAR SYMBOL ':' type exp ')' { $$ = build_var(@1, VAR_DECL, $3, $5, $6); }
| '(' DEFVAR SYMBOL ':' type ')' { $$ = build_var(@1, VAR_DECL, $3, $5, NULL); }
;

include: '(' INCLUDE string_list ')' { $$ = build_include(@1, $3); };


body:
  body_expr { $$ = $1; }
| declare_expr body_expr { $$ = append_tree($2, $1); }
;

body_expr:
  %empty { $$ = NULL; }
| exp body { $$ = append_tree($2, $1); }
| control_stmt body { $$ = append_tree($2, $1); }
| let_stmt body { $$ = append_tree($2, $1); }
;

/*arglist:
       '(' arglist_fields ')' { $$ = $2; }
;

arglist_fields:
              %empty { $$ = NULL; }
| '(' SYMBOL ':' type ')' arglist_fields { $$ = append_tree($6, build_var(PARM_DECL, $2, $4, NULL)); }
;*/

type: modified_typename { $$ = build_type_expr(@1, $1); }
| '*' type { $$ = $2; add_type_ptr($$, SINGLE_PTR, 0); }
| '[' ']' type { $$ = $3; add_type_ptr($$, MULTI_PTR, 0);}
| '[' INTEGER ']' type { $$ = $4; add_type_ptr($$, SIZED_PTR, $2); }
;

typename: SYMBOL { $$ = $1; }
| I8 { $$ = strdup("char"); }
| I16 { $$ = strdup("short"); }
| I32 { $$ = strdup("int"); }
| I64 { $$ = strdup("long"); }
| I128 { $$ = strdup("long long"); }
| U8 { $$ = strdup("unsigned char"); }
| U16 { $$ = strdup("unsigned short"); }
| U32 { $$ = strdup("unsigned int"); }
| U64 { $$ = strdup("unsigned long"); }
| U128 { $$ = strdup("unsigned long long"); }
| F32 { $$ = strdup("float"); }
| F64 { $$ = strdup("double"); }
| F128 { $$ = strdup("long double"); }
| C32 { $$ = strdup("float complex"); }
| C64 { $$ = strdup("double complex"); }
| C128 { $$ = strdup("long double complex"); }
;

modified_typename: typename { $$ = build_tid($1, MOD_NONE); }
| CONST typename { $$ = build_tid($2, MOD_CONST); }
| VOLATILE typename { $$ = build_tid($2, MOD_VOLATILE); }
| RESTRICT typename { $$ = build_tid($2, MOD_RESTRICT); }
| ATOMIC typename { $$ = build_tid($2, MOD_ATOMIC); }

if_stmt:
       '(' IF condition exp ')' { $$ = build_if_else_stmt(@1, $3, $4, NULL); }
| '(' IF condition exp exp ')' { $$ = build_if_else_stmt(@1, $3, $4, $5); }
;

cond_stmt: '(' COND '(' cond_body ')' ')' { $$ = build_cond_stmt(@1, $4); };
cond_body:
         '(' condition body ')' { $$ = build_cond_body(@1, $2, $3); }
| '(' condition body ')' cond_body { $$ = append_tree($5, build_cond_body(@1, $2, $3)); }
;

case_stmt: '(' CASE exp '(' case_body ')' ')' { $$ = build_case_stmt(@1, $3, $5); };
case_body:
         '(' exp body ')' { $$ = build_case_body(@1, $2, $3); }
| '(' exp body ')' case_body { $$ = append_tree($5, build_case_body(@1, $2, $3)); }
;


while_stmt: '(' WHILE condition body ')' {$$ = build_while_stmt(@1, WHILE_STMT, $3, $4);};

do_while_stmt: '(' DOWHILE condition body ')' {$$ = build_while_stmt(@1, DOWHILE_STMT, $3, $4);};

for_stmt: '(' FOR '(' for_assign_body ')' condition exp body ')' { $$ = build_for_stmt(@1, NOTYPE, $4, $6, $7, $8);};
for_assign_body:
               %empty { $$ = NULL; }
| SYMBOL for_assign_body { $$ = append_tree($2, build_var(@1, VAR_DECL, $1, NOTYPE, NULL));}
| '(' SYMBOL exp ')' for_assign_body { $$ = append_tree($5, build_var(@1, VAR_DECL, $2, NOTYPE, $3));}
;

control_stmt: if_stmt {$$ = $1;} 
| cond_stmt{$$ = $1;}
| case_stmt {$$ = $1;}
| while_stmt {$$ = $1;}
| do_while_stmt {$$ = $1;}
| for_stmt{$$ = $1;};

let_stmt: '(' LET '(' let_arg_list ')' body ')' {$$ = build_let_stmt(@1, $4, $6);};
let_arg_list:
  '(' SYMBOL exp ')' { $$ = build_var(@1, VAR_DECL, $2, NOTYPE, $3);  }
| '(' SYMBOL exp ')' let_arg_list { $$ = append_tree($5, build_var(@1, VAR_DECL, $2, NOTYPE, $3)); }
;

lambda_list: '(' lambda_regular_args lambda_optional_args lambda_rest_arg lambda_key_args lambda_aux_args ')' { $$ = build_lambda_list(@1, $2, $3, $4, $5, $6); };

lambda_regular_args:
  %empty { $$ = NULL; }
| SYMBOL lambda_regular_args { $$ = append_tree($2, build_var(@1, PARM_DECL, $1, NOTYPE, NULL)); }
;

lambda_optional_args:
  %empty { $$ = NULL; }
| OPTIONAL lambda_optional_body { $$ = $2; }
;

lambda_optional_body:
  '(' SYMBOL exp ')' { $$ = build_var(@1, PARM_DECL, $2, NOTYPE, $3); }
| '(' SYMBOL exp ')' lambda_optional_body { $$ = append_tree($5, build_var(@1, PARM_DECL, $2, NOTYPE, $3)); }
;

lambda_rest_arg:
  %empty { $$ = NULL; }
| REST SYMBOL { $$ = build_var_ref(@2, $2); }
;


lambda_key_args:
  %empty { $$ = NULL; }
| KEY lambda_key_body { $$ = $2; }
;

lambda_key_body:
  '(' SYMBOL SYMBOL ')' { $$ = build_var(@1, PARM_DECL, $3, NOTYPE, NULL); free($2);}
| '(' SYMBOL SYMBOL ')' lambda_key_body { $$ = append_tree($5, build_var(@1, PARM_DECL, $3, NOTYPE, NULL)); free($2);}
;

lambda_aux_args:
  %empty { $$ = NULL; }
| AUX lambda_aux_body { $$ = $2; }
;

lambda_aux_body:
  SYMBOL { $$ = build_var(@1, PARM_DECL, $1, NOTYPE, NULL); }
| '(' SYMBOL exp ')' { $$ = build_var(@1, PARM_DECL, $2, NOTYPE, $3);}
| SYMBOL lambda_aux_body { $$ = append_tree($2, build_var(@1, PARM_DECL, $1, NOTYPE, NULL)); }
| '(' SYMBOL exp ')' lambda_aux_body { $$ = append_tree($5, build_var(@1, PARM_DECL, $2, NOTYPE, $3));}
;

declare_expr: '(' DECLARE declarations ')' { $$ = $3; };
declarations:
  '(' TYPE type symbol_list ')' { $$ = build_type_decl(@1, $3, $4); }
| '(' TYPE type symbol_list ')' declarations { $$ = append_tree($6, build_type_decl(@1, $3, $4)); }
;

symbol_list:
  SYMBOL { $$ = build_var_ref(@1, $1); }
| SYMBOL symbol_list { $$ = append_tree($2, build_var_ref(@1, $1)); }
;

string_list:
  STRING { $$ = build_string_cst(@1, $1); }
| STRING string_list { $$ = append_tree($2, build_string_cst(@1, $1)); }
;

%%

const char* lcc_current_file = "stdin";

void lccerror (void *locp, char const * err) {
LCCLTYPE* llocp = locp;
  errorat("%s", lcc_current_file, llocp->first_line, llocp->first_column, err);
}

extern FILE *c_in;

int main(int argc, char *const argv[]) {
  argc--;
  argv++;
  if(argc > 0) {
    lcc_current_file = argv[0];
    fprintf(stderr, "Open %s\n", argv[0]);
    lccin = fopen(argv[0], "r");
  }
  else lccin = stdin;

  n_errors = 0;
  int ret = lccparse();


  //head = reverse_tree(head);
  resolve_pass(head);

  if(n_errors > 0) {
    error("compiler generated %d error(s)", n_errors);
    destroy_tree(head);
    return 1;
  }
  print_tree(head);
  destroy_tree(head);

  /*if(argc > 1) {
    c_main(argv[1]);
  }*/
  return ret;
}
