#pragma once

#include <stdbool.h>

enum tree_type {
  FN_DECL,
  PARM_DECL,
  VAR_DECL,

  TYPE_EXPR,
  REFERENCE_EXPR,
  SET_EXPR,
  DEREF_EXPR,
  ADDR_EXPR,
  BINOP_EXPR,
  COMPARE_EXPR,
  COND_EXPR,
  CASE_EXPR,

  LET_STMT,

  WHILE_STMT,
  DOWHILE_STMT,
  FOR_STMT,

  IF_STMT,
  COND_STMT,
  CASE_STMT,

  INCLUDE_STMT,
};

enum type_mod {
  MOD_NONE,
  MOD_CONST,
  MOD_VOLATILE,
  MOD_RESTRICT,
  MOD_ATOMIC,
};

enum reference_expr_type {
  INTEGER_CST,
  FLOAT_CST,
  STRING_CST,
  CHAR_CST,
  BOOL_CST,
  VAR_REF,
  FN_CALL,
};

enum compare_op {
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  OP_EQL,
  OP_NOT,
  OP_AND,
  OP_OR,
};

struct type_id {
  char *name;
  enum type_mod modifier;
};

enum type_ptr_type {
  SINGLE_PTR,
  MULTI_PTR,
  SIZED_PTR,
};

struct type_ptr {
  enum type_ptr_type type;
  struct type_ptr *next;
  int size;
};

struct tree {
  enum tree_type type;
  struct tree *next;

  union {
    struct {
      char *name;
      struct tree *type;
      struct tree *arglist;
      struct tree *body;
    } fn_decl;

    struct {
      char *name;
      struct tree *type;
      struct tree *value;
    } var_decl;

    struct {
      struct type_id *id;
      struct type_ptr *ptr;
    } type_expr;

    struct {
      enum reference_expr_type type;
      union {
        bool bval;
        char cval;
        int ival;
        double fval;
        char *symbol;
        struct {
          char *name;
          struct tree *args;
        } call;
      };
    } reference_expr;

    struct {
      struct tree *vars;
      struct tree *body;
    } let_stmt;

    struct {
      char *name;
      struct tree *value;
    } set_expr;

    struct {
      struct tree *condition;
      struct tree *body;
    } while_stmt;

    struct {
      struct tree *type;
      struct tree *vars;
      struct tree *condition;
      struct tree *loop_eval;
      struct tree *body;
    } for_stmt;

    struct {
      char *path;
    } include;

    struct {
      struct tree *expr;
    } ref_expr;

    struct {
      char op;
      struct tree *body;
    } binop_expr;

    struct {
      enum compare_op op;
      struct tree *lhs;
      struct tree *rhs;
    } compare_expr;

    struct {
      struct tree *condition;
      struct tree *if_block;
      struct tree *else_block;
    } if_else_stmt;

    struct {
      struct tree *condition;
      struct tree *body;
    } cond_expr;

    struct {
      struct tree *exprs;
    } cond_stmt;

    struct {
      struct tree *expr;
      struct tree *body;
    } case_expr;

    struct {
      struct tree *expr;
      struct tree *cases;
    } case_stmt;
  };
};

struct tree *reverse_tree(struct tree *t);

struct tree *build_fn(char *name, struct tree *ret_type, struct tree *arglist,
                      struct tree *body);
struct tree *build_var(enum tree_type type, char *name, struct tree *var_type,
                       struct tree *value);
struct tree *build_type_expr(struct type_id *id);
struct tree *build_set_expr(char *name, struct tree *value);
struct tree *build_binop(char binop, struct tree *body);
struct tree *build_compare(enum compare_op op, struct tree *lhs,
                           struct tree *rhs);

struct tree *build_let_stmt(struct tree *vars, struct tree *body);
struct tree *build_while_stmt(enum tree_type while_type, struct tree *condition,
                              struct tree *body);
struct tree *build_for_stmt(struct tree *type, struct tree *vars,
                            struct tree *condition, struct tree *loop_eval,
                            struct tree *body);
struct tree *build_if_else_stmt(struct tree *condition, struct tree *if_block,
                                struct tree *else_block);
struct tree *build_cond_body(struct tree *condition, struct tree *body);
struct tree *build_cond_stmt(struct tree *exprs);
struct tree *build_case_body(struct tree *expr, struct tree *body);
struct tree *build_case_stmt(struct tree *expr, struct tree *cases);

struct tree *build_int_cst(int value);
struct tree *build_char_cst(char value);
struct tree *build_bool_cst(bool value);
struct tree *build_string_cst(char *value);
struct tree *build_float_cst(double value);

struct tree *build_var_ref(char *var_name);
struct tree *build_fn_call(char *fn_name, struct tree *args);

struct tree *build_include(char *path);
struct tree *build_deref(struct tree *expr);
struct tree *build_addr(struct tree *expr);

struct tree *append_tree(struct tree *t, struct tree *next);

struct type_id *build_tid(char *name, enum type_mod mod);
void destroy_tid(struct type_id *tid);
void destroy_type_ptr(struct type_ptr *ptr);

void add_type_ptr(struct tree *type, enum type_ptr_type ptr_type, int size);
void destroy_tree(struct tree *t);
void print_tree(struct tree *t);

void translate_to_var_name(char *var_name);
int get_bool(struct tree *t); // -1 -> not a bool, 0 -> false, 1 -> false
