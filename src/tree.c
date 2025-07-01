#include "tree.h"
#include "debug.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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
  return t;
}

struct tree *build_fn(char *name, struct tree *type, struct tree *arglist,
                      struct tree *body) {
  struct tree *fn = alloc_tree(1);
  fn->type = FN_DECL;

  translate_to_var_name(name);
  fn->fn_decl.name = name;
  fn->fn_decl.arglist = arglist;
  fn->fn_decl.type = type;
  fn->fn_decl.body = body;

  // info("fn %s :", name);

  return fn;
}

struct tree *build_var(enum tree_type type, char *name, struct tree *var_type,
                       struct tree *value) {
  switch (type) {
  case PARM_DECL:
  case VAR_DECL:
    break;
  default:
    error("Expected PARM_DECL, VAR_DECL for build_var");
    return NULL;
  }
  struct tree *var = alloc_tree(1);
  var->type = type;

  translate_to_var_name(name);
  var->var_decl.name = name;
  var->var_decl.type = var_type;
  var->var_decl.value = value;

  // info("var %s :", name);

  return var;
}

struct tree *build_type_expr(struct type_id *id) {
  struct tree *type = alloc_tree(1);
  type->type = TYPE_EXPR;
  // info("type: %s", id->name);

  type->type_expr.id = id;
  return type;
}

void _destroy_tree(struct tree *t) {
  if (t == NULL)
    return;

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
    if (t->set_expr.name != NULL)
      free(t->set_expr.name);
    destroy_tree(t->set_expr.value);
    break;
  case DEREF_EXPR:
  case ADDR_EXPR:
    destroy_tree(t->ref_expr.expr);
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
    if (t->include.path != NULL)
      free(t->include.path);
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
    fprintf(stderr, "  ");
    _print_tree(body);
    fprintf(stderr, ";\n");
  }
}

static void _print_tree(struct tree *t) {
  if (t == NULL) {
    fprintf(stderr, "(null)");
    return;
  }

  switch (t->type) {
  case FN_DECL:
    _print_tree(t->fn_decl.type);
    fprintf(stderr, " %s", t->fn_decl.name);
    fprintf(stderr, " (");
    for (struct tree *arg = t->fn_decl.arglist; arg != NULL; arg = arg->next) {
      _print_tree(arg);
      if (arg->next == NULL)
        break;
      fprintf(stderr, ", ");
    }
    fputc(')', stderr);

    if (t->fn_decl.body == NULL) {
      fputc(';', stderr);
      break;
    }

    fprintf(stderr, "{\n");
    /*for (struct tree *body = t->fn_decl.body; body != NULL; body = body->next)
    { fprintf(stderr, "  "); _print_tree(body); fprintf(stderr, ";\n");
    }*/
    _print_body(t->fn_decl.body);
    fputc('}', stderr);
    break;
  case PARM_DECL:
  case VAR_DECL:
    _print_tree(t->var_decl.type);
    fprintf(stderr, " %s", t->var_decl.name);
    if (t->var_decl.value == NULL)
      break;
    fprintf(stderr, " = ");
    _print_tree(t->var_decl.value);
    break;
  case TYPE_EXPR:
    switch (t->type_expr.id->modifier) {
    case MOD_CONST:
      fprintf(stderr, "const ");
      break;
    case MOD_VOLATILE:
      fprintf(stderr, "volatile ");
      break;
    case MOD_RESTRICT:
      fprintf(stderr, "restrict ");
      break;
    case MOD_ATOMIC:
      fprintf(stderr, "atomic ");
      break;
    case MOD_NONE:
      break;
    }
    fprintf(stderr, "%s", t->type_expr.id->name);
    for (struct type_ptr *ptr = t->type_expr.ptr; ptr != NULL;
         ptr = ptr->next) {
      switch (ptr->type) {
      case SINGLE_PTR:
        fputc('*', stderr);
        break;
      case MULTI_PTR:
        fprintf(stderr, "[]");
        break;
      case SIZED_PTR:
        fprintf(stderr, "[%d]", ptr->size);
        break;
      }
    }
    break;
  case SET_EXPR:
    fprintf(stderr, "%s = ", t->set_expr.name);
    _print_tree(t->set_expr.value);
    break;
  case DEREF_EXPR:
    fprintf(stderr, "(*");
    _print_tree(t->ref_expr.expr);
    fprintf(stderr, ")");
    break;
  case ADDR_EXPR:
    fprintf(stderr, "(&");
    _print_tree(t->ref_expr.expr);
    fprintf(stderr, ")");
    break;
  case BINOP_EXPR:
    fprintf(stderr, "(");
    for (struct tree *body = t->binop_expr.body; body != NULL;
         body = body->next) {
      _print_tree(body);
      if (body->next == NULL)
        break;
      fprintf(stderr, " %c ", t->binop_expr.op);
    }
    fprintf(stderr, ")");
    break;
  case COMPARE_EXPR:
    if (t->compare_expr.op == OP_NOT)
      fputc('!', stderr);
    fprintf(stderr, "(");
    _print_tree(t->compare_expr.lhs);
    switch (t->compare_expr.op) {
    case OP_LT:
      fprintf(stderr, " < ");
      break;
    case OP_LE:
      fprintf(stderr, " <= ");
      break;
    case OP_GT:
      fprintf(stderr, " > ");
      break;
    case OP_GE:
      fprintf(stderr, " >= ");
      break;
    case OP_EQL:
      fprintf(stderr, " == ");
      break;
    case OP_AND:
      fprintf(stderr, " && ");
      break;
    case OP_OR:
      fprintf(stderr, " || ");
      break;
    default:
      break;
    }
    if (t->compare_expr.op == OP_NOT) {
      fprintf(stderr, ")");
      break;
    }
    _print_tree(t->compare_expr.rhs);
    fprintf(stderr, ")");
    break;
  case COND_EXPR:
    if (get_bool(t->cond_expr.condition) == 1) {
      fprintf(stderr, "{\n");
    } else {
      fprintf(stderr, "if(");
      _print_tree(t->cond_expr.condition);
      fprintf(stderr, ") {\n");
    }
    _print_body(t->cond_expr.body);
    fprintf(stderr, "}\n");
    break;
  case COND_STMT:
    for (struct tree *expr = t->cond_stmt.exprs; expr != NULL;
         expr = expr->next) {
      _print_tree(expr);
      if (expr->next == NULL)
        break;
      fprintf(stderr, "else ");
    }
    break;
  case CASE_EXPR:
    if (get_bool(t->case_expr.expr) == 1) {

      fprintf(stderr, "default:;\n");
    } else {
      fprintf(stderr, "case ");
      _print_tree(t->case_expr.expr);
      fprintf(stderr, ":;\n");
    }

    _print_body(t->case_expr.body);
    fprintf(stderr, "  break;\n");
    break;
  case CASE_STMT:
    fprintf(stderr, "switch(");
    _print_tree(t->case_stmt.expr);
    fprintf(stderr, ") {\n");

    for (struct tree *body = t->case_stmt.cases; body != NULL;
         body = body->next) {
      _print_tree(body);
    }
    fprintf(stderr, "  }\n");
    break;
  case LET_STMT:
    fprintf(stderr, "/* let */ {\n");
    for (struct tree *var = t->let_stmt.vars; var != NULL; var = var->next) {
      fprintf(stderr, "  ");
      _print_tree(var);
      fprintf(stderr, ";\n");
    }

    /*for (struct tree *body = t->let_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stderr, "  ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->let_stmt.body);
    fputc('}', stderr);
    break;
  case WHILE_STMT:
    fprintf(stderr, "while(");
    _print_tree(t->while_stmt.condition);
    fprintf(stderr, ") {\n");

    /*for (struct tree *body = t->while_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stderr, "    ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->while_stmt.body);
    fprintf(stderr, "  }");
    break;
  case DOWHILE_STMT:
    fprintf(stderr, "do {\n");
    /*for (struct tree *body = t->while_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stderr, "    ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->while_stmt.body);
    fprintf(stderr, "  } while(");
    _print_tree(t->while_stmt.condition);
    fprintf(stderr, ")");
    break;
  case FOR_STMT:
    fprintf(stderr, "for(");
    _print_tree(t->for_stmt.type);
    fprintf(stderr, " ");
    for (struct tree *var = t->for_stmt.vars; var != NULL; var = var->next) {
      _print_tree(var);
      if (var->next == NULL)
        break;
      fprintf(stderr, ", ");
    }

    fprintf(stderr, "; ");
    _print_tree(t->for_stmt.condition);
    fprintf(stderr, "; ");
    _print_tree(t->for_stmt.loop_eval);

    fprintf(stderr, ") {\n");
    /*for (struct tree *body = t->for_stmt.body; body != NULL;
         body = body->next) {
      fprintf(stderr, "    ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->for_stmt.body);
    fprintf(stderr, "  }");
    break;
  case IF_STMT:
    fprintf(stderr, "if(");
    _print_tree(t->if_else_stmt.condition);
    fprintf(stderr, ") {\n");
    /*for (struct tree *body = t->if_else_stmt.if_block; body != NULL;
         body = body->next) {
      fprintf(stderr, "    ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->if_else_stmt.if_block);
    fprintf(stderr, "  }");
    if (t->if_else_stmt.else_block == NULL)
      break;
    fprintf(stderr, " else {\n");
    /*for (struct tree *body = t->if_else_stmt.else_block; body != NULL;
         body = body->next) {
      fprintf(stderr, "    ");
      _print_tree(body);
      fprintf(stderr, ";\n");
    }*/
    _print_body(t->if_else_stmt.else_block);
    fprintf(stderr, "  }");

    break;
  case REFERENCE_EXPR:
    switch (t->reference_expr.type) {
    case BOOL_CST:
      fprintf(stderr, t->reference_expr.bval ? "true" : "false");
      break;
    case INTEGER_CST:
      fprintf(stderr, "%d", t->reference_expr.ival);
      break;
    case FLOAT_CST:
      fprintf(stderr, "%g", t->reference_expr.fval);
      break;
    case STRING_CST:
      fprintf(stderr, "%s", t->reference_expr.symbol);
      break;
    case CHAR_CST:
      fprintf(stderr, "'%c'", t->reference_expr.cval);
      break;
    case VAR_REF:
      fprintf(stderr, "%s", t->reference_expr.symbol);
      break;
    case FN_CALL:
      fprintf(stderr, "%s(", t->reference_expr.call.name);
      for (struct tree *arg = t->reference_expr.call.args; arg != NULL;
           arg = arg->next) {
        _print_tree(arg);
        if (arg->next == NULL)
          break;
        fprintf(stderr, ", ");
      }
      fputc(')', stderr);
      break;
    }
    break;
  case INCLUDE_STMT:
    fprintf(stderr, "#include %s", t->include.path);
    break;
  default:
    fprintf(stderr, "(unknown)");
    break;
  }
}
void print_tree(struct tree *t) {
  for (struct tree *head = t; head != NULL; head = head->next) {
    _print_tree(head);
    fputc('\n', stderr);
  }
}

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

struct tree *build_int_cst(int value) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = INTEGER_CST;
  cst->reference_expr.ival = value;
  return cst;
}
struct tree *build_char_cst(char value) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = CHAR_CST;
  cst->reference_expr.cval = value;

  // info("char literal: 0x%02x '%c'", value, value);
  return cst;
}
struct tree *build_bool_cst(bool value) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = BOOL_CST;
  cst->reference_expr.bval = value;
  return cst;
}
struct tree *build_string_cst(char *value) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = STRING_CST;
  cst->reference_expr.symbol = value;
  return cst;
}
struct tree *build_float_cst(double value) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = FLOAT_CST;
  cst->reference_expr.fval = value;
  return cst;
}
struct tree *build_var_ref(char *var_name) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = VAR_REF;
  translate_to_var_name(var_name);
  cst->reference_expr.symbol = var_name;
  return cst;
}
struct tree *build_fn_call(char *fn_name, struct tree *args) {
  struct tree *cst = alloc_tree(1);
  cst->type = REFERENCE_EXPR;
  cst->reference_expr.type = FN_CALL;
  translate_to_var_name(fn_name);
  cst->reference_expr.call.name = fn_name;
  cst->reference_expr.call.args = args;
  return cst;
}

struct tree *build_let_stmt(struct tree *vars, struct tree *body) {
  struct tree *let = alloc_tree(1);
  let->type = LET_STMT;
  let->let_stmt.vars = vars;
  let->let_stmt.body = body;
  return let;
}

struct tree *build_set_expr(char *name, struct tree *value) {
  struct tree *set = alloc_tree(1);
  set->type = SET_EXPR;
  translate_to_var_name(name);
  set->set_expr.name = name;
  set->set_expr.value = value;
  return set;
}

struct tree *build_while_stmt(enum tree_type while_type, struct tree *condition,
                              struct tree *body) {
  switch (while_type) {
  case WHILE_STMT:
  case DOWHILE_STMT:
    // info("while statement");
    break;
  default:
    error("Expected WHILE_STMT or DOWHILE_STMT for build_while_stmt");
    return NULL;
  }
  struct tree *loop = alloc_tree(1);
  loop->type = while_type;
  loop->while_stmt.condition = condition;
  loop->while_stmt.body = body;
  return loop;
}

struct tree *build_for_stmt(struct tree *type, struct tree *vars,
                            struct tree *condition, struct tree *loop_eval,
                            struct tree *body) {
  struct tree *loop = alloc_tree(1);
  loop->type = FOR_STMT;
  loop->for_stmt.type = type;
  loop->for_stmt.vars = vars;
  loop->for_stmt.condition = condition;
  loop->for_stmt.loop_eval = loop_eval;
  loop->for_stmt.body = body;
  return loop;
}

struct tree *build_include(char *path) {
  struct tree *include = alloc_tree(1);
  include->type = INCLUDE_STMT;
  include->include.path = path;
  return include;
}

struct tree *build_deref(struct tree *expr) {
  struct tree *ref = alloc_tree(1);
  ref->type = DEREF_EXPR;
  ref->ref_expr.expr = expr;
  return ref;
}

struct tree *build_addr(struct tree *expr) {
  struct tree *ref = alloc_tree(1);
  ref->type = ADDR_EXPR;
  ref->ref_expr.expr = expr;
  return ref;
}

struct tree *build_binop(char op, struct tree *body) {
  struct tree *binop = alloc_tree(1);
  binop->type = BINOP_EXPR;
  binop->binop_expr.op = op;
  binop->binop_expr.body = body;
  return binop;
}

struct tree *build_compare(enum compare_op op, struct tree *lhs,
                           struct tree *rhs) {
  struct tree *compare = alloc_tree(1);
  compare->type = COMPARE_EXPR;
  compare->compare_expr.op = op;
  compare->compare_expr.lhs = lhs;
  compare->compare_expr.rhs = rhs;
  return compare;
}

struct tree *build_if_else_stmt(struct tree *condition, struct tree *if_block,
                                struct tree *else_block) {
  struct tree *if_else = alloc_tree(1);
  if_else->type = IF_STMT;
  if_else->if_else_stmt.condition = condition;
  if_else->if_else_stmt.if_block = if_block;
  if_else->if_else_stmt.else_block = else_block;
  return if_else;
}

struct tree *build_cond_body(struct tree *condition, struct tree *body) {
  struct tree *cond_body = alloc_tree(1);
  cond_body->type = COND_EXPR;
  cond_body->cond_expr.body = body;
  cond_body->cond_expr.condition = condition;
  return cond_body;
}
struct tree *build_cond_stmt(struct tree *exprs) {
  struct tree *cond_stmt = alloc_tree(1);
  cond_stmt->type = COND_STMT;
  cond_stmt->cond_stmt.exprs = exprs;
  return cond_stmt;
}

struct tree *build_case_body(struct tree *expr, struct tree *body) {
  struct tree *case_expr = alloc_tree(1);
  case_expr->type = CASE_EXPR;
  case_expr->case_expr.expr = expr;
  case_expr->case_expr.body = body;
  return case_expr;
}

struct tree *build_case_stmt(struct tree *expr, struct tree *cases) {
  struct tree *case_stmt = alloc_tree(1);
  case_stmt->type = CASE_STMT;
  case_stmt->case_stmt.expr = expr;
  case_stmt->case_stmt.cases = cases;
  return case_stmt;
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

void translate_to_var_name(char *var_name) {
  for (char *vn = var_name; *vn != '\0'; vn++) {
    if (*vn == '-')
      *vn = '_';
  }
}
