#ifndef VARIABLES_H
#define VARIABLES_H

#include "utils.h"

struct var {
    char *name;
    char *value;
};

void init_vars();
char *var_get(const char *name);
void var_set(const char *name, const char *value);
void expand_vars(int argc, char **argv);
void free_vars();

#endif
