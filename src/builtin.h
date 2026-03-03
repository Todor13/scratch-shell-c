#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "history.h"

typedef int (*builtin_fn)(int argc, char **argv);

struct builtin
{
  const char *name;
  builtin_fn fn;
};

int builtin_echo(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_type(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_cd(int argc, char **argv);
int builtin_history(int argc, char **argv);

extern struct builtin builtins[];

#endif
