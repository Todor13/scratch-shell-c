#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tokenizer.h"

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

void redirect(struct tokenize_result *result)
{
  if (result->redirect != REDIRECT_PIPE) {
    int fd = open(result->redirect_path,
                  O_WRONLY | O_CREAT | (result->redirect_mode == OVERRIDE ? O_TRUNC : O_APPEND),
                  0644);
    if (result->redirect == REDIRECT_STDOUT)
      dup2(fd, STDOUT_FILENO);
    if (result->redirect == REDIRECT_STDERR)
      dup2(fd, STDERR_FILENO);
    close(fd);
  }
}

void pipe_setup(struct tokenize_result *result)
{
  if (result->prev_read != -1)
    dup2(result->prev_read, STDIN_FILENO);

  if (result->current_pipe < result->n_pipes - 1)
    dup2(result->pipefd[1], STDOUT_FILENO);

  if (result->prev_read != -1)
    close(result->prev_read);

  if (result->current_pipe < result->n_pipes - 1) {
    close(result->pipefd[0]);
    close(result->pipefd[1]);
  }
}

void pipe_close(struct tokenize_result *result)
{
  if (result->prev_read != -1)
    close(result->prev_read);

  if (result->current_pipe < result->n_pipes - 1) {
    close(result->pipefd[1]);
    result->prev_read = result->pipefd[0];
  }
}

int dispatch_builtin(struct tokenize_result *result)
{
  for (int i = 0; builtins[i].name; i++) {
    if (strcmp(result->argv[0], builtins[i].name) == 0) {
      if (result->redirect == REDIRECT_STDERR || result->redirect == REDIRECT_STDOUT) {
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);
        redirect(result);

        int status = builtins[i].fn(result->argc, result->argv);

        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);

        return status;
      } else if (result->redirect == REDIRECT_PIPE) {
        pid_t pid = fork();
        if (pid == 0) {
          pipe_setup(result);
          builtins[i].fn(result->argc, result->argv);
          _exit(127);
        }
        result->pids[result->current_pipe] = pid;
        return 0;
      } else {
        return builtins[i].fn(result->argc, result->argv);
      }
    }
  }
  return -1;
}

int dispatch_executable(struct tokenize_result *result)
{
  char fullpath[1024];
  if (find_executable(result->argv[0], fullpath, sizeof fullpath) == 0) {
    char *cmd = result->argv[0];
    pid_t pid = fork();
    if (pid == 0) {
      if (result->redirect == REDIRECT_STDERR || result->redirect == REDIRECT_STDOUT) {
        redirect(result);
      } else if (result->redirect == REDIRECT_PIPE) {
        pipe_setup(result);
      }

      if (strchr(cmd, '/')) {
        execv(cmd, result->argv);
      } else {
        execvp(cmd, result->argv);
      }
      perror(cmd);
      _exit(127);
    }
    if (pid < 0) {
      perror("fork");
      return 1;
    }
    result->pids[result->current_pipe] = pid;

    int status;
    if (result->redirect != REDIRECT_PIPE && waitpid(pid, &status, 0) < 0) {
      perror("waitpid");
      return 1;
    }

    return 0;
  }

  return -1;
}

int dispatch(struct tokenize_result *result)
{
  int status;
  if (result->redirect == REDIRECT_PIPE) {
    for (int i = 0; i < result->n_pipes; i++) {
      result->current_pipe = i;
      result->argc = result->pipe_args[i].argc;
      int j = 0;
      for (j; j < result->argc; j++) {
        result->argv[j] = result->pipe_args[i].argv[j];
      }
      result->argv[j] = NULL;

      if (pipe(result->pipefd) < 0) {
        perror("pipe");
        exit(1);
      }

      int s;
      if ((s = dispatch_builtin(result)) > -1) {
        status = s;
        pipe_close(result);
        continue;
      }

      if ((s = dispatch_executable(result)) > -1) {
        status = s;
      }

      pipe_close(result);
    }

    for (int i = 0; i < result->n_pipes; i++) {
      int st;
      waitpid(result->pids[i], &st, 0);
    }
    
    return status;
  }

  if ((status = dispatch_builtin(result)) > -1) {
    return status;
  }

  if ((status = dispatch_executable(result)) > -1) {
    return status;
  }

  return -1;
}

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  char *line = NULL;
  size_t len = 0;
  __ssize_t nread;

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

    struct tokenize_result *result = tokenize(line);

    if (dispatch(result) == -1) {
      printf("%s: command not found\n", line);
    }

    // destruct_result(result);
  }

  free(line);
  return 0;
}
