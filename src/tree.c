#include "tree.h"
#include "debug.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern const char *lcc_current_file;

struct tree_reverser {
  struct tree *head;
  struct tree *tail;
};

static struct tree_reverser *reverser_helper(struct tree *tree) {
  if (tree == NULL)
    return NULL;
  if (tree->next == NULL) {
    struct tree_reverser *result = calloc(1, sizeof(struct tree_reverser));
    result->head = tree;
    result->tail = tree;
    return result;
  }

  struct tree_reverser *chain = reverser_helper(tree->next);
  chain->tail->next = tree;
  chain->tail = tree;
  tree->next = NULL;
  return chain;
};

struct tree *reverse_tree(struct tree *tree) {
  if (tree == NULL)
    return NULL;

  struct tree_reverser *chain = reverser_helper(tree);
  struct tree *new_head = chain->head;
  free(chain);
  return new_head;
}

struct tree *alloc_tree(size_t n) {
  struct tree *t = calloc(n, sizeof(struct tree));
  t->valid = true;
  return t;
}

struct tree *build_fn(struct location loc, char *name, struct tree *type,
                      struct tree *arglist, struct tree *body) {
  struct tree *fn = alloc_tree(1);
  fn->loc = loc;
  fn->type = FN_DECL;

  translate_to_var_name(name);
  fn->fn_decl.name = name;
  fn->fn_decl.arglist = arglist;
  fn->fn_decl.type = type;
  fn->fn_decl.body = body;

  // info("fn %s :", name);

  return fn;
}

struct tree *build_var(struct location loc, enum tree_type type, char *name,
                       struct tree *var_type, struct tree *value) {
  switch (type) {
  case PARM_DECL:
  case VAR_DECL:
    break;
  default:
    errorat("Expected PARM_DECL, VAR_DECL for build_var", lcc_current_file,
            loc.first_line, loc.first_column);
    return NULL;
  }
  struct tree *var = alloc_tree(1);
  var->loc = loc;
  var->type = type;

  translate_to_var_name(name);
  var->var_decl.name = name;
  var->var_decl.type = var_type;
  var->var_decl.value = value;

  // info("var %s :", name);

  return var;
}

struct tree *build_type_expr(struct location loc, struct type_id *id) {
  struct tree *type = alloc_tree(1);
  type->type = TYPE_EXPR;
  type->loc = loc;

  type->type_expr.id = id;
  return type;
}

void _destroy_tree(struct tree *t) {
  if (t == NULL)
    return;
  if (!t->valid)
    return;
  t->valid = false;

  switch (t->type) {
  case FN_DECL:
    if (t->fn_decl.name)
      free(t->fn_decl.name);

    destroy_tree(t->fn_decl.body);
    destroy_tree(t->fn_decl.arglist);
    destroy_tree(t->fn_decl.type);
    break;
  case PARM_DECL:
  case VAR_DECL:
    if (t->var_decl.name)
      free(t->var_decl.name);
    destroy_tree(t->var_decl.type);
    destroy_tree(t->var_decl.value);
    break;
  case TYPE_EXPR:
    destroy_tid(t->type_expr.id);
    destroy_type_ptr(t->type_expr.ptr);
    break;
  case SET_EXPR:
    destroy_tree(t->set_expr.var);
    destroy_tree(t->set_expr.value);
    break;
  case AREF_EXPR:
  case ADDR_EXPR:
    destroy_tree(t->ref_expr.indices);
    destroy_tree(t->ref_expr.expr);
    break;
  case CAST_EXPR:
    destroy_tree(t->cast_expr.type);
    destroy_tree(t->cast_expr.expr);
    break;
  case BINOP_EXPR:
    destroy_tree(t->binop_expr.body);
    break;
  case COMPARE_EXPR:
    destroy_tree(t->compare_expr.lhs);
    destroy_tree(t->compare_expr.rhs);
    break;
  case COND_EXPR:
    destroy_tree(t->cond_expr.condition);
    destroy_tree(t->cond_expr.body);
    break;
  case COND_STMT:
    destroy_tree(t->cond_stmt.exprs);
    break;
  case CASE_EXPR:
    destroy_tree(t->case_expr.expr);
    destroy_tree(t->case_expr.body);
    break;
  case CASE_STMT:
    destroy_tree(t->case_stmt.expr);
    destroy_tree(t->case_stmt.cases);
    break;
  case LET_STMT:
    destroy_tree(t->let_stmt.vars);
    destroy_tree(t->let_stmt.body);
    break;
  case WHILE_STMT:
  case DOWHILE_STMT:
    destroy_tree(t->while_stmt.condition);
    destroy_tree(t->while_stmt.body);
    break;
  case FOR_STMT:
    destroy_tree(t->for_stmt.type);
    destroy_tree(t->for_stmt.vars);
    destroy_tree(t->for_stmt.condition);
    destroy_tree(t->for_stmt.loop_eval);
    destroy_tree(t->for_stmt.body);
    break;
  case IF_STMT:
    destroy_tree(t->if_else_stmt.condition);
    destroy_tree(t->if_else_stmt.if_block);
    destroy_tree(t->if_else_stmt.else_block);
    break;
  case REFERENCE_EXPR:
    switch (t->reference_expr.type) {
    case STRING_CST:
    case VAR_REF:
      if (t->reference_expr.symbol != NULL)
        free(t->reference_expr.symbol);
      break;
    case FN_CALL:
      if (t->reference_expr.call.name != NULL)
        free(t->reference_expr.call.name);
      destroy_tree(t->reference_expr.call.args);
    default:
      break;
    }
    break;
  case INCLUDE_STMT:
    destroy_tree(t->include_stmt.paths);
    break;
  case LAMBDA_LIST:
    destroy_tree(t->lambda_list.args);
    destroy_tree(t->lambda_list.optionals);
    destroy_tree(t->lambda_list.rest);
    destroy_tree(t->lambda_list.keys);
    destroy_tree(t->lambda_list.aux);
    break;
  case LAMBDA_KEY:
    if (t->lambda_key.key_name)
      free(t->lambda_key.key_name);
    destroy_tree(t->lambda_key.expr);
    break;
  case TYPE_DECL:
    destroy_tree(t->type_decl.type);
    destroy_tree(t->type_decl.symbol_list);
    break;
  default:
    warning("Destroying unimplemented tree type %d", t->type);
    break;
  }

  free(t);
}

void destroy_tree(struct tree *t) {
  for (struct tree *head = t, *next; head != NULL; head = next) {
    next = head->next;
    head->next = NULL;

    _destroy_tree(head);
  }
}

struct tree *append_tree(struct tree *t, struct tree *next) {
  if (next == NULL)
    return t;
  struct tree *last;
  for (last = next; last->next != NULL; last = last->next)
    ;
  last->next = t;
  return next;
}

static void _print_tree(struct tree *t);
static void _print_body(struct tree *t) {
  for (struct tree *body = t; body != NULL; body = body->next) {
    fprintf(stdout, "  ");
    _print_tree(body);
    fprintf(stdout, ";\n");
  }
}

static void _print_tree(struct tree *t) {
  if (t == NULL) {
    fprintf(stdout, "(null)");
    return;
  }

  switch (t->type) {
  case FN_DECL:
    _print_tree(t->fn_decl.type);
    fprintf(stdout, " %s", t->fn_decl.name);
    fprintf(stdout, " (");
    struct tree *args = t->fn_decl.arglist;
    if (args != NULL) {
      if (args->type == LAMBDA_LIST) {
        int n_lists = (args->lambda_list.args == NULL ? 0 : 1) +
                      (args->lambda_list.optionals == NULL ? 0 : 1) +
                      (args->lambda_list.rest == NULL ? 0 : 1) +
                      (args->lambda_list.keys == NULL ? 0 : 1);
        for (struct tree *arg = args->lambda_list.args; arg != NULL;
             arg = arg->next) {
          _print_tree(arg);
          if (arg->next == NULL && (--n_lists) <= 0)
            break;
          fprintf(stdout, ", ");
        }
        for (struct tree *arg = args->lambda_list.optionals; arg != NULL;
             arg = arg->next) {
          if (arg->type != PARM_DECL) {
            error("expected parm_decl");
            continue;
          }
          _print_tree(arg->var_decl.type);
          fprintf(stdout, " %s", arg->var_decl.name);
          if (arg->next == NULL && (--n_lists) <= 0)
            break;
          fprintf(stdout, ", ");
        }
        for (struct tree *arg = args->lambda_list.keys; arg != NULL;
             arg = arg->next) {
          if (arg->type != LAMBDA_KEY) {
            error("expected lambda_key");
            continue;
          }
          _print_tree(arg->lambda_key.expr->var_decl.type);
          fprintf(stdout, " %s", arg->lambda_key.expr->var_decl.name);
          if (arg->next == NULL && (--n_lists) <= 0)
            break;
          fprintf(stdout, ", ");
        }

        if (args->lambda_list.rest != NULL) {
          fprintf(stdout, "...");
        }
      } else {
        errorat("expected lambda_list but received %s", lcc_current_file,
                args->loc.first_line, args->loc.first_column,
                get_tree_type(args));
      }
    }
    fputc(')', stdout);

    if (t->fn_decl.body == NULL && args->lambda_list.aux == NULL) {
      fputc(';', stdout);
      break;
    }

    fprintf(stdout, "{\n");
    _print_body(args->lambda_list.aux);
    /*for (struct tree *body = t->fn_decl.body; body != NULL; body = body->next)
    { fprintf(stdout, "  "); _print_tree(body); fprintf(stdout, ";\n");
    }*/
    _print_body(t->fn_decl.body);
    fputc('}', stdout);
    break;
  case PARM_DECL:
  case VAR_DECL:
    _print_tree(t->var_decl.type);
    fprintf(stdout, " %s", t->var_decl.name);
    if (t->var_decl.value == NULL)
      break;
    fprintf(stdout, " = ");
    _print_tree(t->var_decl.value);
    break;
  case TYPE_EXPR:
    switch (t->type_expr.id->modifier) {
    case MOD_CONST:
      fprintf(stdout, "const ");
      break;
    case MOD_VOLATILE:
      fprintf(stdout, "volatile ");
      break;
    case MOD_RESTRICT:
      fprintf(stdout, "restrict ");
      break;
    case MOD_ATOMIC:
      fprintf(stdout, "atomic ");
      break;
    case MOD_NONE:
    case MOD_MONOMORPH:
      break;
    }
    if (t->type_expr.id != NULL && t->type_expr.id->name != NULL) {
      fprintf(stdout, "%s", t->type_expr.id->name);
    } else {
      fprintf(stdout, "(null)");
    }
    for (struct type_ptr *ptr = t->type_expr.ptr; ptr != NULL;
         ptr = ptr->next) {
      switch (ptr->type) {
      case SINGLE_PTR:
        fputc('*', stdout);
        break;
      case MULTI_PTR:
        fprintf(stdout, "[]");
        break;
      case SIZED_PTR:
        fprintf(stdout, "[%d]", ptr->size);
        break;
      }
    }
    break;
  case SET_EXPR:
    _print_tree(t->set_expr.var);
    switch (t->set_expr.mod) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '|':
    case '&':
    case '~':
      fprintf(stdout, " %c= ", t->set_expr.mod);
      break;
    default:
      fprintf(stdout, " = ");
      break;
    }
    _print_tree(t->set_expr.value);
    break;
  case AREF_EXPR:
    if (t->ref_expr.indices == NULL) {
      fprintf(stdout, "(*");
    }
    _print_tree(t->ref_expr.expr);

    if (t->ref_expr.indices == NULL) {
      fprintf(stdout, ")");
      break;
    }
    for (struct tree *index = t->ref_expr.indices; index != NULL;
         index = index->next) {
      fputc('[', stdout);
      _print_tree(index);
      fputc(']', stdout);
    }
    break;
  case ADDR_EXPR:
    fprintf(stdout, "(&");
    _print_tree(t->ref_expr.expr);
    fprintf(stdout, ")");
    break;
  case CAST_EXPR:
    fprintf(stdout, "((");
    _print_tree(t->cast_expr.type);
    fprintf(stdout, ")");
    _print_tree(t->cast_expr.expr);
    fprintf(stdout, ")");
    break;
    break;
  case BINOP_EXPR:
    fprintf(stdout, "(");
    for (struct tree *body = t->binop_expr.body; body != NULL;
         body = body->next) {
      _print_tree(body);
      if (body->next == NULL)
        break;
      fprintf(stdout, " %c ", t->binop_expr.op);
    }
    fprintf(stdout, ")");
    break;
  case COMPARE_EXPR:
    if (t->compare_expr.op == OP_NOT)
      fputc('!', stdout);
    fprintf(stdout, "(");
    _print_tree(t->compare_expr.lhs);
    switch (t->compare_expr.op) {
    case OP_LT:
      fprintf(stdout, " < ");
      break;
    case OP_LE:
      fprintf(stdout, " <= ");
      break;
    case OP_GT:
      fprintf(stdout, " > ");
      break;
    case OP_GE:
      fprintf(stdout, " >= ");
      break;
    case OP_EQL:
      fprintf(stdout, " == ");
      break;
    case OP_AND:
      fprintf(stdout, " && ");
      break;
    case OP_OR:
      fprintf(stdout, " || ");
      break;
    default:
      break;
    }
    if (t->compare_expr.op == OP_NOT) {
      fprintf(stdout, ")");
      break;
    }
    _print_tree(t->compare_expr.rhs);
    fprintf(stdout, ")");
    break;
  case COND_EXPR:
    if (get_bool(t->cond_expr.condition) == 1) {
      fprintf(stdout, "{\n");
    } else {
      fprintf(stdout, "if(");
      _print_tree(t->cond_expr.condition);
      fprintf(stdout, ") {\n");
    }
    _print_body(t->cond_expr.body);
    fprintf(stdout, "}\n");
    break;
  case COND_STMT:
    for (struct tree *expr = t->cond_stmt.exprs; expr != NULL;
         expr = expr->next) {
      _print_tree(expr);
      if (expr->next == NULL)
        break;
      fprintf(stdout, "else ");
    }
    break;
  case CASE_EXPR:
    if (get_bool(t->case_expr.expr) == 1) {

      fprintf(stdout, "default:;\n");
    } else {
      fprintf(stdout, "case ");
      _print_tree(t->case_expr.expr);
      fprintf(stdout, ":;\n");
    }

    _print_body(t->case_expr.body);
    fprintf(stdout, "  break;\n");
    break;
  case CASE_STMT:
    fprintf(stdout, "switch(");
    _print_tree(t->case_stmt.expr);
    fprintf(stdout, ") {\n");

    for (struct tree *body = t->case_stmt.cases; body != NULL;
         body = body->next) {
      _print_tree(body);
    }
    fprintf(stdout, "  }\n");
    break;
  case LET_STMT:
    fprintf(stdout, "/* let */ {\n");
    for (struct tree *var = t->let_stmt.vars; var != NULL; var = var->next) {
      fprintf(stdout, "  ");
      _print_tree(var);
      fprintf(stdout, ";\n");
    }

    /*for (struct tree *body = t->let_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stdout, "  ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->let_stmt.body);
    fputc('}', stdout);
    break;
  case WHILE_STMT:
    fprintf(stdout, "while(");
    _print_tree(t->while_stmt.condition);
    fprintf(stdout, ") {\n");

    /*for (struct tree *body = t->while_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stdout, "    ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->while_stmt.body);
    fprintf(stdout, "  }");
    break;
  case DOWHILE_STMT:
    fprintf(stdout, "do {\n");
    /*for (struct tree *body = t->while_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stdout, "    ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->while_stmt.body);
    fprintf(stdout, "  } while(");
    _print_tree(t->while_stmt.condition);
    fprintf(stdout, ")");
    break;
  case FOR_STMT:
    fprintf(stdout, "for(");
    //_print_tree(t->for_stmt.type);
    fprintf(stdout, " ");
    for (struct tree *var = t->for_stmt.vars; var != NULL; var = var->next) {
      _print_tree(var);
      if (var->next == NULL)
        break;
      fprintf(stdout, ", ");
    }

    fprintf(stdout, "; ");
    _print_tree(t->for_stmt.condition);
    fprintf(stdout, "; ");
    _print_tree(t->for_stmt.loop_eval);

    fprintf(stdout, ") {\n");
    /*for (struct tree *body = t->for_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stdout, "    ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->for_stmt.body);
    fprintf(stdout, "  }");
    break;
  case IF_STMT:
    fprintf(stdout, "if(");
    _print_tree(t->if_else_stmt.condition);
    fprintf(stdout, ") {\n");
    /*for (struct tree *body = t->if_else_stmt.if_block; body != NULL;
         body = body->next) {
      fprintf(stdout, "    ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->if_else_stmt.if_block);
    fprintf(stdout, "  }");
    if (t->if_else_stmt.else_block == NULL)
      break;
    fprintf(stdout, " else {\n");
    /*for (struct tree *body = t->if_else_stmt.else_block; body != NULL;
         body = body->next) {
      fprintf(stdout, "    ");
      _print_tree(body);
      fprintf(stdout, ";\n");
    }*/
    _print_body(t->if_else_stmt.else_block);
    fprintf(stdout, "  }");

    break;
  case REFERENCE_EXPR:
    switch (t->reference_expr.type) {
    case BOOL_CST:
      fprintf(stdout, t->reference_expr.bval ? "true" : "false");
      break;
    case INTEGER_CST:
      fprintf(stdout, "%d", t->reference_expr.ival);
      break;
    case FLOAT_CST:
      fprintf(stdout, "%g", t->reference_expr.fval);
      break;
    case STRING_CST:
      fprintf(stdout, "%s", t->reference_expr.symbol);
      break;
    case CHAR_CST:
      fprintf(stdout, "'%c'", t->reference_expr.cval);
      break;
    case VAR_REF:
      fprintf(stdout, "%s", t->reference_expr.symbol);
      break;
    case FN_CALL:
      fprintf(stdout, "%s(", t->reference_expr.call.name);
      for (struct tree *arg = t->reference_expr.call.args; arg != NULL;
           arg = arg->next) {
        _print_tree(arg);
        if (arg->next == NULL)
          break;
        fprintf(stdout, ", ");
      }
      fputc(')', stdout);
      break;
    }
    break;
  case INCLUDE_STMT:
    for (struct tree *path = t->include_stmt.paths; path != NULL;
         path = path->next) {
      fprintf(stdout, "#include ");
      _print_tree(path);
      fputc('\n', stdout);
    }
    break;
  case LAMBDA_LIST:
    for (struct tree *head = t->lambda_list.args; head != NULL;
         head = head->next) {
      _print_tree(head);
      if (head->next == NULL)
        break;
      fprintf(stdout, ", ");
    }
    break;
  case TYPE_DECL:
    break;
  default:
    fprintf(stdout, "(unknown)");
    break;
  }
}
void print_tree(struct tree *t) {
  for (struct tree *head = t; head != NULL; head = head->next) {
    _print_tree(head);
    fputc(';', stdout);
    fputc('\n', stdout);
  }
}
#define A(B, c) B c

A(int, dontuse);

struct type_id *build_tid(char *name, enum type_mod mod) {
  struct type_id *tid = calloc(1, sizeof(struct type_id));
  translate_to_var_name(name);
  tid->name = name;
  tid->modifier = mod;

  return tid;
}

void destroy_tid(struct type_id *tid) {
  if (tid == NULL)
    return;
  if (tid->name)
    free(tid->name);
  free(tid);
}

void add_type_ptr(struct tree *type, enum type_ptr_type ptr_type, int size) {
  if (type == NULL)
    return;
  if (type->type != TYPE_EXPR) {
    error("expected type_expr for add_type_ptr, received %d", type->type);
    return;
  }
  struct type_ptr *tp = calloc(1, sizeof(struct type_ptr));
  tp->type = ptr_type;
  tp->size = size;
  tp->next = type->type_expr.ptr;
  type->type_expr.ptr = tp;
}

void destroy_type_ptr(struct type_ptr *ptr) {
  if (ptr == NULL)
    return;
  if (ptr->next != NULL)
    destroy_type_ptr(ptr->next);
  free(ptr);
}

struct tree *build_int_cst(struct location loc, int value) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = INTEGER_CST;
  cst->reference_expr.ival = value;
  return cst;
}
struct tree *build_char_cst(struct location loc, char value) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = CHAR_CST;
  cst->reference_expr.cval = value;

  // info("char literal: 0x%02x '%c'", value, value);
  return cst;
}
struct tree *build_bool_cst(struct location loc, bool value) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = BOOL_CST;
  cst->reference_expr.bval = value;
  return cst;
}
struct tree *build_string_cst(struct location loc, char *value) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = STRING_CST;
  cst->reference_expr.symbol = value;
  return cst;
}
struct tree *build_float_cst(struct location loc, double value) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = FLOAT_CST;
  cst->reference_expr.fval = value;
  return cst;
}
struct tree *build_var_ref(struct location loc, char *var_name) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = VAR_REF;
  translate_to_var_name(var_name);
  cst->reference_expr.symbol = var_name;
  return cst;
}
struct tree *build_fn_call(struct location loc, char *fn_name,
                           struct tree *args) {
  struct tree *cst = alloc_tree(1);
  cst->loc = loc;
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = FN_CALL;
  translate_to_var_name(fn_name);
  cst->reference_expr.call.name = fn_name;
  cst->reference_expr.call.args = args;
  return cst;
}

struct tree *build_let_stmt(struct location loc, struct tree *vars,
                            struct tree *body) {
  struct tree *let = alloc_tree(1);
  let->loc = loc;
  let->type = LET_STMT;
  let->let_stmt.vars = vars;
  let->let_stmt.body = body;
  return let;
}

struct tree *build_set_expr(struct location loc, struct tree *var,
                            struct tree *value, char mod) {
  struct tree *set = alloc_tree(1);
  set->loc = loc;
  set->type = SET_EXPR;
  set->set_expr.var = var;
  set->set_expr.value = value;
  set->set_expr.mod = mod;
  return set;
}

struct tree *build_while_stmt(struct location loc, enum tree_type while_type,
                              struct tree *condition, struct tree *body) {
  switch (while_type) {
  case WHILE_STMT:
  case DOWHILE_STMT:
    // info("while statement");
    break;
  default:
    errorat("Expected WHILE_STMT or DOWHILE_STMT for build_while_stmt",
            lcc_current_file, loc.first_line, loc.first_column);
    return NULL;
  }
  struct tree *loop = alloc_tree(1);
  loop->loc = loc;
  loop->type = while_type;
  loop->while_stmt.condition = condition;
  loop->while_stmt.body = body;
  return loop;
}

struct tree *build_for_stmt(struct location loc, struct tree *type,
                            struct tree *vars, struct tree *condition,
                            struct tree *loop_eval, struct tree *body) {
  struct tree *loop = alloc_tree(1);
  loop->loc = loc;
  loop->type = FOR_STMT;
  loop->for_stmt.type = type;
  loop->for_stmt.vars = vars;
  loop->for_stmt.condition = condition;
  loop->for_stmt.loop_eval = loop_eval;
  loop->for_stmt.body = body;
  return loop;
}

struct tree *build_include(struct location loc, struct tree *paths) {
  struct tree *include = alloc_tree(1);
  include->loc = loc;
  include->type = INCLUDE_STMT;
  include->include_stmt.paths = paths;
  return include;
}

struct tree *build_aref(struct location loc, struct tree *expr,
                        struct tree *indices) {
  struct tree *ref = alloc_tree(1);
  ref->loc = loc;
  ref->type = AREF_EXPR;
  ref->ref_expr.expr = expr;
  ref->ref_expr.indices = indices;
  return ref;
}

struct tree *build_addr(struct location loc, struct tree *expr) {
  struct tree *ref = alloc_tree(1);
  ref->loc = loc;
  ref->type = ADDR_EXPR;
  ref->ref_expr.expr = expr;
  return ref;
}

struct tree *build_binop(struct location loc, char op, struct tree *body) {
  struct tree *binop = alloc_tree(1);
  binop->loc = loc;
  binop->type = BINOP_EXPR;
  binop->binop_expr.op = op;
  binop->binop_expr.body = body;
  return binop;
}

struct tree *build_compare(struct location loc, enum compare_op op,
                           struct tree *lhs, struct tree *rhs) {
  struct tree *compare = alloc_tree(1);
  compare->loc = loc;
  compare->type = COMPARE_EXPR;
  compare->compare_expr.op = op;
  compare->compare_expr.lhs = lhs;
  compare->compare_expr.rhs = rhs;
  return compare;
}

struct tree *build_if_else_stmt(struct location loc, struct tree *condition,
                                struct tree *if_block,
                                struct tree *else_block) {
  struct tree *if_else = alloc_tree(1);
  if_else->loc = loc;
  if_else->type = IF_STMT;
  if_else->if_else_stmt.condition = condition;
  if_else->if_else_stmt.if_block = if_block;
  if_else->if_else_stmt.else_block = else_block;
  return if_else;
}

struct tree *build_cond_body(struct location loc, struct tree *condition,
                             struct tree *body) {
  struct tree *cond_body = alloc_tree(1);
  cond_body->loc = loc;
  cond_body->type = COND_EXPR;
  cond_body->cond_expr.body = body;
  cond_body->cond_expr.condition = condition;
  return cond_body;
}
struct tree *build_cond_stmt(struct location loc, struct tree *exprs) {
  struct tree *cond_stmt = alloc_tree(1);
  cond_stmt->loc = loc;
  cond_stmt->type = COND_STMT;
  cond_stmt->cond_stmt.exprs = exprs;
  return cond_stmt;
}

struct tree *build_case_body(struct location loc, struct tree *expr,
                             struct tree *body) {
  struct tree *case_expr = alloc_tree(1);
  case_expr->loc = loc;
  case_expr->type = CASE_EXPR;
  case_expr->case_expr.expr = expr;
  case_expr->case_expr.body = body;
  return case_expr;
}

struct tree *build_case_stmt(struct location loc, struct tree *expr,
                             struct tree *cases) {
  struct tree *case_stmt = alloc_tree(1);
  case_stmt->loc = loc;
  case_stmt->type = CASE_STMT;
  case_stmt->case_stmt.expr = expr;
  case_stmt->case_stmt.cases = cases;
  return case_stmt;
}

struct tree *build_inc(struct location loc, struct tree *var) {
  return build_set_expr(loc, var, build_int_cst(loc, 1), '+');
}

struct tree *build_dec(struct location loc, struct tree *var) {
  return build_set_expr(loc, var, build_int_cst(loc, 1), '-');
}

struct tree *build_lambda_list(struct location loc, struct tree *args,
                               struct tree *optionals, struct tree *rest,
                               struct tree *keys, struct tree *aux) {
  struct tree *list = alloc_tree(1);
  list->loc = loc;
  list->type = LAMBDA_LIST;
  list->lambda_list.args = args;
  list->lambda_list.optionals = optionals;
  list->lambda_list.rest = rest;
  list->lambda_list.keys = keys;
  list->lambda_list.aux = aux;

  return list;
}

struct tree *build_type_decl(struct location loc, struct tree *type,
                             struct tree *symbol_list) {
  struct tree *type_decl = alloc_tree(1);
  type_decl->loc = loc;
  type_decl->type = TYPE_DECL;
  type_decl->type_decl.type = type;
  type_decl->type_decl.symbol_list = symbol_list;
  return type_decl;
}

struct tree *build_cast(struct location loc, struct tree *type,
                        struct tree *expr) {

  struct tree *cast_expr = alloc_tree(1);
  cast_expr->loc = loc;
  cast_expr->type = CAST_EXPR;
  cast_expr->cast_expr.type = type;
  cast_expr->cast_expr.expr = expr;
  return cast_expr;
}

struct tree *build_lambda_key(struct location loc, char *key,
                              struct tree *expr) {
  struct tree *lambda_key = alloc_tree(1);
  lambda_key->loc = loc;
  lambda_key->type = LAMBDA_KEY;
  lambda_key->lambda_key.key_name = key;
  lambda_key->lambda_key.expr = expr;
  return lambda_key;
}

int get_bool(struct tree *t) {
  if (t == NULL)
    return -1;
  switch (t->type) {
  case REFERENCE_EXPR:
    switch (t->reference_expr.type) {
    case BOOL_CST:
      return t->reference_expr.bval ? 1 : 0;
      break;
    default:
      return -1;
    }
  default:
    return -1;
  }
}

int get_int(struct tree *t) {
  if (t == NULL)
    return 0;
  switch (t->type) {
  case REFERENCE_EXPR:
    switch (t->reference_expr.type) {
    case INTEGER_CST:
      return t->reference_expr.ival;
      break;
    default:
      return 0;
    }
  default:
    return 0;
  }
}

void translate_to_var_name(char *var_name) {
  if (var_name == NULL)
    return;
  for (char *vn = var_name; *vn != '\0'; vn++) {
    if (*vn == '-')
      *vn = '_';
  }
}

const char *get_tree_type(struct tree *t) {
  if (t == NULL)
    return "%%null%%";
  switch (t->type) {
#define DEFTREECODE(NAME, STR, ...)                                            \
  case NAME:                                                                   \
    return #STR;
#include "tree.def"
#undef DEFTREECODE
  default:
    return "%%unknown%%";
  }
}

bool is_monomorph(struct tree *t) {
  if (t == NULL)
    return false;
  if (t->type != TYPE_EXPR)
    return false;
  if (t->type_expr.id == NULL)
    return false;
  if (t->type_expr.id->modifier == MOD_MONOMORPH)
    return true;
  return false;
}

static void copy_to_type_id(struct type_id *dest, struct type_id *src) {
  if (dest == NULL || src == NULL)
    return;
  dest->modifier = src->modifier;
  dest->name = strdup(src->name);
}

static struct type_ptr *copy_type_ptr(struct type_ptr *ptr) {
  if (ptr == NULL)
    return NULL;
  struct type_ptr *next = NULL;
  if (ptr->next != NULL)
    next = copy_type_ptr(ptr->next);

  struct type_ptr *copy = calloc(1, sizeof(struct type_ptr));
  copy->type = ptr->type;
  copy->size = ptr->size;
  copy->next = next;
  return copy;
}

void copy_type_to_type(struct tree *dest, struct tree *src) {
  if (dest == NULL || src == NULL)
    return;
  if (dest->type != TYPE_EXPR || src->type != TYPE_EXPR)
    return;
  dest->loc = src->loc;
  copy_to_type_id(dest->type_expr.id, src->type_expr.id);
  destroy_type_ptr(dest->type_expr.ptr);
  dest->type_expr.ptr = copy_type_ptr(src->type_expr.ptr);
}
