#include "builtin.h"
#include "input.h"
#include "tokenizer.h"

const int MAX_ARGS = 32;

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
  char *cmd = result->argv[0];
  pid_t pid = fork();
  if (pid == 0) {
    if (result->redirect == REDIRECT_STDERR || result->redirect == REDIRECT_STDOUT) {
      redirect(result);
    } else if (result->redirect == REDIRECT_PIPE) {
      pipe_setup(result);
    }

    execvp(cmd, result->argv);
    exit(127);
  }
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  result->pids[result->current_pipe] = pid;

  int status;
  if (result->redirect != REDIRECT_PIPE) {
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      int code = WEXITSTATUS(status);
      if (code == 127) {
        return -1;
      }
    }
  }

  return 0;
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
  load_env_history();

  for (;;) {
    line = read_line();

    if (line[0] != '\0') {
      write_history(strdup(line));

      struct tokenize_result *result = tokenize(line);

      if (dispatch(result) == -1) {
        printf("%s: command not found\n", line);
      }

      free_tokenize_result(result);
    }
  }

  free(line);
  return 0;
}
