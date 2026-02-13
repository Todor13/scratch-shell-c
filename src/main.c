#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int MAX_ARGS = 32;

typedef int (*builtin_fn)(int argc, char **argv);

struct builtin
{
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

int builtin_pwd(int argc, char **argv)
{
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    return 1;
  }

  printf("%s\n", cwd);
  return 0;
}

int builtin_type(int argc, char **argv);

int builtin_exit(int argc, char **argv)
{
  exit(0);
}

int builtin_cd(int argc, char **argv)
{
  char *dir;
  if (strchr(argv[1], '~')) {
    char *home = getenv("HOME");
    if (home == NULL) {
      printf("HOME dir not set\n");
      return 1;
    }

    dir = home;
  } else {
    dir = argv[1];
  }

  if (chdir(dir) == -1) {
    switch (errno) {
    case ENOENT:
      printf("%s: No such file or directory\n", dir);
      return 1;

    case ENOTDIR:
      printf("%s: Not a directory\n", dir);
      return 1;

    case EACCES:
      printf("%s: Permission denied\n", dir);
      return 1;

    default:
      return 1;
    }
  }

  return 0;
}

// clang-format off
struct builtin builtins[] = {
  { "echo", builtin_echo }, 
  { "type", builtin_type }, 
  { "exit", builtin_exit }, 
  { "pwd", builtin_pwd },
  { "cd", builtin_cd },
  { NULL, NULL }
};
// clang-format on

int find_executable(const char *cmd, char *out, size_t outsz)
{
  if (!cmd)
    return 1;

  const char *path = getenv("PATH");
  if (!path)
    return 1;

  char *copy = strdup(path);
  if (!copy)
    return 1;

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
  return 1;
}

int dispatch_builtin(int argc, char **argv)
{
  for (int i = 0; builtins[i].name; i++) {
    if (strcmp(argv[0], builtins[i].name) == 0) {
      return builtins[i].fn(argc, argv);
    }
  }

  return -1;
}

int dispatch_executable(int argc, char **argv)
{
  char fullpath[1024];
  if (find_executable(argv[0], fullpath, sizeof fullpath) == 0) {
    char *cmd = argv[0];
    pid_t pid = fork();
    if (pid == 0) {
      if (strchr(cmd, '/')) {
        execv(cmd, argv);
      } else {
        execvp(cmd, argv);
      }
      perror(cmd);
      _exit(127);
    }
    if (pid < 0) {
      perror("fork");
      return 1;
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      perror("waitpid");
      return 1;
    }

    return 0;
  }

  return -1;
}

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

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  char *line = NULL;
  size_t len = 0;
  __ssize_t nread;
  int status = 0;

  for (;;) {
    printf("$ ");
    nread = getline(&line, &len, stdin);

    if (nread == -1) {
      perror("getline");
      free(line);
      return 1;
    }

    if (nread > 0 && line[nread - 1] == '\n')
      line[nread - 1] = '\0';

    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv);

    if ((status = dispatch_builtin(argc, argv)) > -1) {
      continue;
    }

    if ((status = dispatch_executable(argc, argv)) > -1) {
      continue;
    }

    printf("%s: command not found\n", line);
  }

  free(line);
  return 0;
}
