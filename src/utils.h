#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>

void *xmalloc(size_t size);
void * xrealloc(void *ptr, size_t size);
int parse_int(const char *s, int *out);

#endif