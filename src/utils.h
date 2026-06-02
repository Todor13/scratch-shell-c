#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xcalloc(size_t count, size_t size);
char *xstrdup(const char *s);
int parse_int(const char *s, int *out);
char *build_cmd(char **argv, int argc);

#endif