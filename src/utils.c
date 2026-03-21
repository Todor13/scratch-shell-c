#include "utils.h"

void *xmalloc(size_t size)
{
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

int parse_int(const char *s, int *out)
{
  char *end;
  long parsed = strtol(s, &end, 10);
  if (s == end || *end != '\0')
    return 0;

  *out = (int)parsed;
  return 1;
}