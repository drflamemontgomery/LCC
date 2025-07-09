#include "debug.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

extern const char *lcc_current_file;

struct hashmap_chain {
  struct hashmap_s map;
  struct hashmap_chain *next;
};

static struct hashmap_chain *create_hashmap_chain(hashmap_uint32_t size,
                                                  struct hashmap_chain *next) {
  struct hashmap_chain *chain = calloc(1, sizeof(struct hashmap_chain));
  if (chain == NULL)
    return NULL;
  if (hashmap_create(size, &chain->map) != 0) {
    error("Failed to create hashmap.");
    free(chain);
    return NULL;
  }
  chain->next = next;

  return chain;
}

static void destroy_hashmap_chain(struct hashmap_chain *chain) {
  if (chain == NULL)
    return;
  hashmap_destroy(&chain->map);
  chain->next = NULL;
  free(chain);
}

static void destroy_hashmap_chain_recurse(struct hashmap_chain *chain) {
  struct hashmap_chain *c = chain;
  while (c != NULL) {
    struct hashmap_chain *next = c->next;
    destroy_hashmap_chain(c);
    c = next;
  }
}

static void resolve_tree(struct tree *t, struct hashmap_chain *env);
static void resolve_tree_chain(struct tree *t, struct hashmap_chain *env) {
  for (struct tree *chain = t; chain != NULL; chain = chain->next) {
    resolve_tree(chain, env);
  }
}

static bool key_exists(char *key, struct hashmap_chain *env) {
  size_t key_size = strlen(key);
  for (struct hashmap_chain *chain = env; chain != NULL; chain = chain->next) {
    if (hashmap_get(&chain->map, key, key_size) != NULL)
      return true;
  }
  return false;
}
static struct tree *hashmap_get_recurse(char *key, struct hashmap_chain *env) {
  size_t key_size = strlen(key);
  struct tree *value;
  for (struct hashmap_chain *chain = env; chain != NULL; chain = chain->next) {
    if ((value = (struct tree *)hashmap_get(&chain->map, key, key_size)) !=
        NULL)
      return value;
  }
  return NULL;
}

static void resolve_fn_decl(struct tree *t, struct hashmap_chain *env) {
  struct hashmap_chain *block_env = create_hashmap_chain(128, env);
  if (hashmap_put(&env->map, t->fn_decl.name, strlen(t->fn_decl.name),
                  (void *)t) != 0) {
    error("failed to add '%s' to hashmap", t->fn_decl.name);
  }

  resolve_tree_chain(t->fn_decl.arglist, block_env);
  resolve_tree_chain(t->fn_decl.body, block_env);

  destroy_hashmap_chain(block_env);
}

static void resolve_reference(struct tree *t, struct hashmap_chain *env) {
  if (t == NULL)
    return;
  if (t->type != REFERENCE_EXPR)
    return;

  switch (t->reference_expr.type) {
  case FN_CALL:;
    struct tree *fn_decl =
        hashmap_get_recurse(t->reference_expr.call.name, env);
    for (struct tree *arg = t->reference_expr.call.args; arg != NULL;
         arg = arg->next) {
      resolve_tree(arg, env);
    }
    if (fn_decl == NULL)
      break;
    if (fn_decl->type != FN_DECL) {
      errorat("%s is not a function", lcc_current_file, t->loc.first_line,
              t->loc.first_column, t->reference_expr.call.name);
      break;
    }
    struct tree *lambda_list = fn_decl->fn_decl.arglist;
    if (lambda_list->type != LAMBDA_LIST) {
      errorat("expected lambda list. Received %s", lcc_current_file,
              t->loc.first_line, t->loc.first_column,
              get_tree_type(lambda_list));
      break;
    }

    int n_regular_args = 0;
    int n_optional_args = 0;
    int n_key_args = 0;
    for (struct tree *arg = lambda_list->lambda_list.args; arg != NULL;
         arg = arg->next) {
      n_regular_args++;
    }
    for (struct tree *arg = lambda_list->lambda_list.optionals; arg != NULL;
         arg = arg->next) {
      n_optional_args++;
    }
    for (struct tree *arg = lambda_list->lambda_list.keys; arg != NULL;
         arg = arg->next) {
      n_key_args++;
    }

    info("Function %s is declared", t->reference_expr.call.name);
    struct tree *keys = NULL, *key_end = NULL;
    struct tree *args = NULL, *arg_end = NULL;
    struct tree *rest = NULL, *rest_end = NULL;
    int n_args = 0, n_keys = 0;
    for (struct tree *arg = t->reference_expr.call.args; arg != NULL;) {
      struct tree *next = arg == NULL ? NULL : arg->next;
      arg->next = NULL;
      if (arg->type == LAMBDA_KEY) {
        if (key_end == NULL)
          keys = key_end = arg;
        else {
          key_end->next = arg;
          key_end = arg;
        }
        n_keys++;
      } else if (n_args < n_regular_args + n_optional_args) {
        if (arg_end == NULL)
          args = arg_end = arg;
        else {
          arg_end->next = arg;
          arg_end = arg;
        }
        n_args++;
      } else {
        if (rest_end == NULL)
          rest = rest_end = arg;
        else {
          rest_end->next = arg;
          rest_end = arg;
        }
      }

      arg = next;
    }

    if (n_args < n_regular_args || n_keys < n_key_args) {
      errorat("incorrect number of arguments for %s", lcc_current_file,
              t->loc.first_line, t->loc.first_column,
              t->reference_expr.call.name);
      break;
    }
    n_args -= n_regular_args;

    int i = 0;
    for (struct tree *opt = lambda_list->lambda_list.optionals; opt != NULL;
         opt = opt->next) {
      if (i < n_args) {
        i++;
        continue;
      }

      struct tree *expr = opt->var_decl.value;
      if (args == NULL) {
        args = arg_end = expr;
        args->next = NULL;
        continue;
      }
      arg_end->next = expr;
      arg_end = arg_end->next;
      arg_end->next = NULL;
    }

    for (struct tree *key = lambda_list->lambda_list.keys; key != NULL;
         key = key->next) {
      if (key->lambda_key.key_name == NULL)
        continue;
      info("Test for key %s", key->lambda_key.key_name);
      for (struct tree *arg = keys; arg != NULL; arg = arg->next) {
        if (arg == NULL)
          continue;
        if (arg->type != LAMBDA_KEY)
          continue;
        if (arg->lambda_key.key_name == NULL)
          continue;
        info("Test against %s", arg->lambda_key.key_name);
        if (strcmp(arg->lambda_key.key_name, key->lambda_key.key_name) == 0) {
          info("Append %s", arg->lambda_key.key_name);
          if (arg_end != NULL) {
            arg_end->next = arg->lambda_key.expr;
            arg_end = arg_end->next;
          } else {
            args = arg_end = arg->lambda_key.expr;
          }
          arg->lambda_key.expr = NULL;
        }
      }
    }
    destroy_tree(keys);

    if (arg_end) {
      arg_end->next = rest;
    } else {
      args = rest;
    }
    t->reference_expr.call.args = args;

    break;
  default:
    warning("Un-implemented resolve for reference type");
    break;
  }
}

static void resolve_tree(struct tree *t, struct hashmap_chain *env) {
  if (t == NULL)
    return;
  struct hashmap_chain *block_env;
  switch (t->type) {
  case FN_DECL:
    resolve_fn_decl(t, env);
    break;
  case LET_STMT:;
    block_env = create_hashmap_chain(128, env);

    resolve_tree_chain(t->let_stmt.vars, block_env);
    resolve_tree_chain(t->let_stmt.body, block_env);

    destroy_hashmap_chain(block_env);
    break;
  case WHILE_STMT:
  case DOWHILE_STMT:
    block_env = create_hashmap_chain(128, env);

    resolve_tree_chain(t->while_stmt.condition, block_env);
    resolve_tree_chain(t->while_stmt.body, block_env);

    destroy_hashmap_chain(block_env);
    break;
  case FOR_STMT:
    block_env = create_hashmap_chain(128, env);

    resolve_tree_chain(t->for_stmt.vars, block_env);
    resolve_tree_chain(t->for_stmt.body, block_env);

    destroy_hashmap_chain(block_env);
    break;
  case COND_STMT:
    resolve_tree_chain(t->cond_stmt.exprs, env);
    break;
  case CASE_STMT:
    block_env = create_hashmap_chain(128, env);

    resolve_tree_chain(t->case_stmt.expr, block_env);
    resolve_tree_chain(t->case_stmt.cases, block_env);

    destroy_hashmap_chain(block_env);
    break;
  case COND_EXPR:
    block_env = create_hashmap_chain(128, env);

    resolve_tree_chain(t->cond_expr.condition, env);
    resolve_tree_chain(t->cond_expr.body, block_env);

    destroy_hashmap_chain(block_env);
    break;
  case CASE_EXPR:
    resolve_tree_chain(t->case_expr.expr, env);
    resolve_tree_chain(t->case_expr.body, env);
    break;
  case LAMBDA_LIST:
    resolve_tree_chain(t->lambda_list.args, env);
    resolve_tree_chain(t->lambda_list.optionals, env);
    resolve_tree_chain(t->lambda_list.rest, env);
    resolve_tree_chain(t->lambda_list.keys, env);
    resolve_tree_chain(t->lambda_list.aux, env);
    break;
  case LAMBDA_KEY:
    resolve_tree(t->lambda_key.expr, env);
    break;
  case VAR_DECL:
  case PARM_DECL:
    if (key_exists(t->var_decl.name, env)) {
      errorat("symbol '%s' already exists", lcc_current_file, t->loc.first_line,
              t->loc.first_column, t->var_decl.name);
      break;
    }
    if (hashmap_put(&env->map, t->var_decl.name, strlen(t->var_decl.name), t) !=
        0) {
      error("failed to add '%s' to hashmap", t->var_decl.name);
    }
    break;
  case TYPE_DECL:;
    struct tree *type = t->type_decl.type;
    for (struct tree *symbol = t->type_decl.symbol_list; symbol != NULL;
         symbol = symbol->next) {
      if (symbol->type != REFERENCE_EXPR ||
          symbol->reference_expr.type != VAR_REF) {
        errorat("expected symbol", lcc_current_file, symbol->loc.first_line,
                symbol->loc.first_column);
        continue;
      }
      struct tree *ref_tree =
          hashmap_get_recurse(symbol->reference_expr.symbol, env);
      if (ref_tree == NULL) {
        errorat("symbol '%s' does not exist", lcc_current_file,
                symbol->loc.first_line, symbol->loc.first_column,
                symbol->reference_expr.symbol);
        continue;
      }
      struct tree *overwrite_type = NULL;
      switch (ref_tree->type) {
      case VAR_DECL:
      case PARM_DECL:
        overwrite_type = ref_tree->var_decl.type;
        break;
      case FN_DECL:
        overwrite_type = ref_tree->fn_decl.type;
        break;
      default:
        errorat("internal error. Symbol '%s' not linked to var_decl or fn_decl",
                lcc_current_file, symbol->loc.first_line,
                symbol->loc.first_column, symbol->reference_expr.symbol);
        continue;
      }
      if (overwrite_type->type != TYPE_EXPR) {
        errorat("internal error. Symbol '%s' not linked to type structure",
                lcc_current_file, symbol->loc.first_line,
                symbol->loc.first_column, symbol->reference_expr.symbol);
        continue;
      }
      copy_type_to_type(overwrite_type, type);
    }
    break;
  case REFERENCE_EXPR:
    resolve_reference(t, env);
    break;
  case BINOP_EXPR:
  case INCLUDE_STMT:
  case COMPARE_EXPR:
    break;
  default:
    warning("Un-implemented resolve for tree type %s", get_tree_type(t));
    break;
  }
}

void resolve_pass(struct tree *t) {
  struct hashmap_chain *env = create_hashmap_chain(1024, NULL);
  if (env == NULL) {
    error("Failed to initialize environment hashmap.");
    return;
  }

  for (struct tree *head = t; head != NULL; head = head->next) {
    resolve_tree(head, env);
  }

  destroy_hashmap_chain_recurse(env);
}
