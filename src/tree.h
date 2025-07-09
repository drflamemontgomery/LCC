#pragma once

#include <stdbool.h>

struct location {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

enum tree_type {
#define DEFTREECODE(NAME, STR, ...) NAME,
#include "tree.def"
#undef DEFTREECODE
};

enum type_mod {
  MOD_NONE,
  MOD_CONST,
  MOD_VOLATILE,
  MOD_RESTRICT,
  MOD_ATOMIC,
  MOD_MONOMORPH,
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
  struct location loc;
  bool valid;

  union {
#define DEFTREECODE(ENUM, ID, ...) __VA_OPT__(struct {__VA_ARGS__} ID;)
#include "tree.def"
#undef DEFTREECODE
  };
};

struct tree *reverse_tree(struct tree *t);

struct tree *build_fn(struct location loc, char *name, struct tree *ret_type,
                      struct tree *arglist, struct tree *body);
struct tree *build_var(struct location loc, enum tree_type type, char *name,
                       struct tree *var_type, struct tree *value);
struct tree *build_type_expr(struct location loc, struct type_id *id);
struct tree *build_set_expr(struct location loc, struct tree *name,
                            struct tree *value, char mod);
struct tree *build_binop(struct location loc, char binop, struct tree *body);
struct tree *build_compare(struct location loc, enum compare_op op,
                           struct tree *lhs, struct tree *rhs);
struct tree *build_inc(struct location loc, struct tree *var);
struct tree *build_dec(struct location loc, struct tree *var);

struct tree *build_let_stmt(struct location loc, struct tree *vars,
                            struct tree *body);
struct tree *build_while_stmt(struct location loc, enum tree_type while_type,
                              struct tree *condition, struct tree *body);
struct tree *build_for_stmt(struct location loc, struct tree *type,
                            struct tree *vars, struct tree *condition,
                            struct tree *loop_eval, struct tree *body);
struct tree *build_if_else_stmt(struct location loc, struct tree *condition,
                                struct tree *if_block, struct tree *else_block);
struct tree *build_cond_body(struct location loc, struct tree *condition,
                             struct tree *body);
struct tree *build_cond_stmt(struct location loc, struct tree *exprs);
struct tree *build_case_body(struct location loc, struct tree *expr,
                             struct tree *body);
struct tree *build_case_stmt(struct location loc, struct tree *expr,
                             struct tree *cases);

struct tree *build_int_cst(struct location loc, int value);
struct tree *build_char_cst(struct location loc, char value);
struct tree *build_bool_cst(struct location loc, bool value);
struct tree *build_string_cst(struct location loc, char *value);
struct tree *build_float_cst(struct location loc, double value);

struct tree *build_var_ref(struct location loc, char *var_name);
struct tree *build_fn_call(struct location loc, char *fn_name,
                           struct tree *args);

struct tree *build_include(struct location loc, struct tree *paths);
struct tree *build_aref(struct location loc, struct tree *expr,
                        struct tree *indices);
struct tree *build_addr(struct location loc, struct tree *expr);
struct tree *build_cast(struct location loc, struct tree *type,
                        struct tree *expr);

struct tree *build_lambda_list(struct location loc, struct tree *args,
                               struct tree *optionals, struct tree *rest,
                               struct tree *keys, struct tree *aux);
struct tree *build_lambda_key(struct location loc, char *key,
                              struct tree *expr);

struct tree *build_type_decl(struct location loc, struct tree *type,
                             struct tree *symbol_list);

struct tree *append_tree(struct tree *t, struct tree *next);

struct type_id *build_tid(char *name, enum type_mod mod);
void destroy_tid(struct type_id *tid);
void destroy_type_ptr(struct type_ptr *ptr);

void add_type_ptr(struct tree *type, enum type_ptr_type ptr_type, int size);
void destroy_tree(struct tree *t);
void print_tree(struct tree *t);

void translate_to_var_name(char *var_name);
int get_bool(struct tree *t); // -1 -> not a bool, 0 -> false, 1 -> false
bool is_monomorph(struct tree *t);

void resolve_pass(struct tree *t);
const char *get_tree_type(struct tree *t);
void copy_type_to_type(struct tree *dest, struct tree *src);
struct tree *copy_tree(struct tree *t);
