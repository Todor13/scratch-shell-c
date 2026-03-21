#include "builtin.h"

// clang-format off
struct builtin builtins[] = {
  { "echo", builtin_echo }, 
  { "type", builtin_type }, 
  { "exit", builtin_exit }, 
  { "pwd", builtin_pwd },
  { "cd", builtin_cd },
  { "history", builtin_history},
  { NULL, NULL }
};
// clang-format on

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

int builtin_exit(int argc, char **argv)
{
  persist_env_history();
  free_history();
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

int builtin_history(int argc, char **argv)
{
  int i = 0;
  int len;
  char **history = read_full_history(&len);
  if (argc > 2 && *argv[1] == '-') {
    if (argv[1][1] == 'r' && argv[2]) {
      read_history_fd(argv[2]);
      return 0;
    } else if ((argv[1][1] == 'w' || argv[1][1] == 'a') && argv[2]) {
      char mode[2] = { argv[1][1], '\0' };
      write_history_fd(argv[2], mode);
      return 0;
    }
    return 1;
  }

  int limit;
  if (argc > 1 && parse_int(argv[1], &limit))
    i = len - limit;

  for (i; i < len; i++) {
    printf("  %d %s\n", i, history[i]);
  }

  return 0;
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
  if (find_executable(argv[1], fullpath, sizeof(fullpath)) == 0) {
    printf("%s is %s\n", argv[1], fullpath);
    return 0;
  }

  printf("%s not found\n", argv[1]);
  return 1;
}
