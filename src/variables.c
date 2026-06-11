#include "variables.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

struct var **vars = NULL;
size_t vars_len = 0;
size_t init_len = 2;

void init_vars() { vars = xmalloc(init_len * sizeof(struct var *)); }

static struct var *find_var(const char *name) {
  for (size_t i = 0; i < vars_len; i++) {
    if (strcmp(name, vars[i]->name) == 0)
      return vars[i];
  }

  return NULL;
}

char *var_get(const char *name) {
  struct var *var = find_var(name);
  return var != NULL ? var->value : NULL;
}

void var_set(const char *name, const char *value) {
  struct var *var = find_var(name);

  if (var != NULL) {
    free(var->value);
    var->value = xstrdup(value);
  } else {
    if (vars_len == init_len) {
      init_len *= 2;
      vars = xrealloc(vars, init_len);
    }

    struct var *new_var = xmalloc(sizeof(struct var));
    new_var->name = xstrdup(name);
    new_var->value = xstrdup(value);
    vars[vars_len++] = new_var;
  }
}

void free_vars() {
  for (size_t i = 0; i < vars_len; i++) {
    free(vars[i]->name);
    free(vars[i]->value);
    free(vars[i]);
  }
  free(vars);
}