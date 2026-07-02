#include "variables.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

struct var **vars = NULL;
size_t vars_len = 0;
size_t init_len = 2;

void init_vars()
{
  vars = xmalloc(init_len * sizeof(struct var *));
}

static struct var *find_var(const char *name)
{
  for (size_t i = 0; i < vars_len; i++) {
    if (strcmp(name, vars[i]->name) == 0)
      return vars[i];
  }

  return NULL;
}

char *var_get(const char *name)
{
  struct var *var = find_var(name);
  return var != NULL ? var->value : NULL;
}

void var_set(const char *name, const char *value)
{
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

void expand_vars(int argc, char **argv)
{
  for (size_t i = 0; i < argc; i++) {
    char buffer[1000];
    int buffer_index = 0;
    int l = -1;
    int r = -1;
    bool found = false;
    for (size_t j = 0; j < strlen(argv[i]); j++) {
      char *arg = argv[i];
      if (arg[j] == '$' || (l != -1 && arg[j] == '{' && arg[j - 1] == '$'))
        l = j;
      else if (l != -1 && arg[j] == '}')
        r = j;
      else if (l == -1)
        buffer[buffer_index++] = arg[j];

      if ((l != -1 && r != -1) || (l != -1 && r == -1 && j + 1 == strlen(argv[i]))) {
        if (r == -1) {
          r = strlen(argv[i]);
        }

        char last = arg[r];
        arg[r] = '\0';
        char *variable = var_get(arg + l + 1);
        if (variable != NULL) {
          strcpy(buffer + buffer_index, variable);
          buffer_index += strlen(variable);
        }
        arg[r] = last;
        l = -1;
        r = -1;
        found = true;
      }
    }

    if (found) {
      buffer[buffer_index + 1] = '\0'; 
      free(argv[i]);
      argv[i] = xstrdup(buffer);
    }
  }
}

void free_vars()
{
  for (size_t i = 0; i < vars_len; i++) {
    free(vars[i]->name);
    free(vars[i]->value);
    free(vars[i]);
  }
  free(vars);
}