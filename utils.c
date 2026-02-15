#include <stdlib.h>
#include <string.h>

const int MAX_ARGS = 32;

int tokenize(char *line, char **argv)
{
  int argc = 0;
  char *tok = strtok(line, " \t");
  while (tok && argc < MAX_ARGS) {
    argv[argc++] = tok;
    tok = strtok(NULL, " \t");
  }

  argv[argc] = NULL;
  return argc;
}