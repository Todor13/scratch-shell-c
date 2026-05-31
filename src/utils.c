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

char *xstrdup(const char *s)
{
  if (s == NULL)
    return NULL;

  size_t len = strlen(s) + 1;
  char *dup = xmalloc(len);
  memcpy(dup, s, len);
  return dup;
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

char *build_cmd(char **argv, int argc)
{
  size_t len = 0;
  for (int i = 0; i < argc; i++) {
    len += strlen(argv[i]) + 1;
  }

  char *cmd = xmalloc((len + 1) * sizeof(char));
  strcpy(cmd, argv[0]);

  for (int i = 1; i < argc; i++) {
    if (i > 0) {
      strcat(cmd, " ");
    }
    strcat(cmd, argv[i]);
  }

  return cmd;
}
