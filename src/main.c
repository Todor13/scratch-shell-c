#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int MAX_ARGS = 32;

typedef int (*builtin_fn)(int argc, char **argv);

struct builtin {
  const char *name;
  builtin_fn fn;
};

int builtin_echo(int argc, char **argv)
{
  for (int i = 1; i < argc; i++) {
    printf("%s", argv[i]);
    if (i + 1 < argc)
      printf(" ");
  }
  printf("\n");
  return 0;
}

int builtin_type(int argc, char **argv);

int builtin_exit(int argc, char **argv) 
{
  exit(0);
}

struct builtin builtins[] = {
  {"echo", builtin_echo},
  {"type", builtin_type},
  {"exit", builtin_exit},
  {NULL, NULL}
};

int find_executable(const char *cmd, char *out, size_t outsz) 
{
  if (!cmd) 
    return -1;

  const char *path = getenv("PATH");
  if (!path) 
    return -1;

  char *copy = strdup(path);
  if (!copy)
    return -1;

  char *saveptr = NULL;
  for (char *dir = __strtok_r(copy, ":", &saveptr); dir; dir = __strtok_r(NULL, ":", &saveptr)) {
    if (*dir == '\0')
      dir = ".";

    int n = snprintf(out, outsz, "%s/%s", dir, cmd);
    if (n < 0 || (size_t)n >= outsz)
      continue;

    if (access(out, X_OK) == 0) {
      free(copy);
      return 0;
    }
  }

  free(copy);
  return -1;
}

int builtin_type(int argc, char **argv) 
{
  for (int i = 0; builtins[i].name; i++) {
    if (strcmp(argv[1], builtins[i].name) == 0) {
      printf("%s is a shell builtin\n", argv[1]);
      return 0;
    }
  }

  char fullpath[1024];
  if (find_executable(argv[1], fullpath, sizeof fullpath) == 0) {
    printf("%s is %s\n", argv[1], fullpath);
    return 0;
  }
  
  printf("%s not found\n", argv[1]);
  return 0;
}

int tokenize(char *line, char **argv) {
  int argc = 0;
  char *tok = strtok(line, " \t");
  while(tok && argc < MAX_ARGS) {
    argv[argc++] = tok;
    tok = strtok(NULL, " \t");
  }

  argv[argc] = NULL;
  return argc;
}

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  char *line = NULL;
  size_t len = 0;
  __ssize_t nread;

  while (1)
  {
    printf("$ ");
    nread = getline(&line, &len, stdin);

    if (nread == -1) {
      perror("getline");
      free(line);
      return 1;
    }

    if (nread > 0 && line[nread - 1] == '\n')
      line[nread - 1] = '\0';

    int result = -1;
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv);
    for (int i = 0; builtins[i].name; i++) {
      if (strcmp(argv[0], builtins[i].name) == 0) {
        result = builtins[i].fn(argc, argv);
        if (result != 0) {
          free(line);
          return result; 
        }
        break;
      }
    }
    
    if (result == -1) {
      printf("%s: command not found\n", line);
    }
  }

  free(line);
  return 0;
}
