#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "history.h"
#include "exec.h"
#include "jobs.h"
#include "variables.h"

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
int builtin_jobs(int argc, char **argv);
int builtin_declare(int argc, char **argv);

extern struct builtin builtins[];

#endif
