#include "input.h"

const int MAX_ARGS = 32;

static void redirect(struct tokenize_ctx *ctx)
{
  if (ctx->redirect != REDIRECT_PIPE) {
    int fd = open(ctx->redirect_path,
                  O_WRONLY | O_CREAT | (ctx->redirect_mode == OVERRIDE ? O_TRUNC : O_APPEND),
                  0644);
    if (ctx->redirect == REDIRECT_STDOUT)
      dup2(fd, STDOUT_FILENO);
    if (ctx->redirect == REDIRECT_STDERR)
      dup2(fd, STDERR_FILENO);
    close(fd);
  }
}

static void pipe_setup(struct tokenize_ctx *ctx)
{
  if (ctx->prev_read != -1)
    dup2(ctx->prev_read, STDIN_FILENO);

  if (ctx->current_pipe < ctx->n_pipes - 1)
    dup2(ctx->pipefd[1], STDOUT_FILENO);

  if (ctx->prev_read != -1)
    close(ctx->prev_read);

  if (ctx->current_pipe < ctx->n_pipes - 1) {
    close(ctx->pipefd[0]);
    close(ctx->pipefd[1]);
  }
}

void pipe_close(struct tokenize_ctx *ctx)
{
  if (ctx->prev_read != -1)
    close(ctx->prev_read);

  if (ctx->current_pipe < ctx->n_pipes - 1) {
    close(ctx->pipefd[1]);
    ctx->prev_read = ctx->pipefd[0];
  }
}

static int dispatch_builtin(struct tokenize_ctx *ctx)
{
  for (int i = 0; builtins[i].name; i++) {
    if (strcmp(ctx->argv[0], builtins[i].name) == 0) {
      if (ctx->redirect == REDIRECT_STDERR || ctx->redirect == REDIRECT_STDOUT) {
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);
        redirect(ctx);

        int status = builtins[i].fn(ctx->argc, ctx->argv);

        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);

        return status;
      } else if (ctx->redirect == REDIRECT_PIPE) {
        pid_t pid = fork();
        if (pid == 0) {
          pipe_setup(ctx);
          builtins[i].fn(ctx->argc, ctx->argv);
          _exit(127);
        }
        ctx->pids[ctx->current_pipe] = pid;
        return 0;
      } else {
        return builtins[i].fn(ctx->argc, ctx->argv);
      }
    }
  }
  return -1;
}

static int dispatch_executable(struct tokenize_ctx *ctx)
{
  char *cmd = ctx->argv[0];
  pid_t pid = fork();
  if (pid == 0) {
    if (ctx->redirect == REDIRECT_STDERR || ctx->redirect == REDIRECT_STDOUT) {
      redirect(ctx);
    } else if (ctx->redirect == REDIRECT_PIPE) {
      pipe_setup(ctx);
    }

    execvp(cmd, ctx->argv);
    exit(127);
  }
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  ctx->pids[ctx->current_pipe] = pid;

  int status;
  if (ctx->redirect != REDIRECT_PIPE) {
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

static int dispatch(struct tokenize_ctx *ctx)
{
  int status;
  if (ctx->redirect == REDIRECT_PIPE) {
    for (int i = 0; i < ctx->n_pipes; i++) {
      ctx->current_pipe = i;
      ctx->argc = ctx->pipe_args[i].argc;
      int j = 0;
      for (j; j < ctx->argc; j++) {
        ctx->argv[j] = ctx->pipe_args[i].argv[j];
      }
      ctx->argv[j] = NULL;

      if (pipe(ctx->pipefd) < 0) {
        perror("pipe");
        exit(1);
      }

      int s;
      if ((s = dispatch_builtin(ctx)) > -1) {
        status = s;
        pipe_close(ctx);
        continue;
      }

      if ((s = dispatch_executable(ctx)) > -1) {
        status = s;
      }

      pipe_close(ctx);
    }

    for (int i = 0; i < ctx->n_pipes; i++) {
      int st;
      waitpid(ctx->pids[i], &st, 0);
    }

    return status;
  }

  if ((status = dispatch_builtin(ctx)) > -1) {
    return status;
  }

  if ((status = dispatch_executable(ctx)) > -1) {
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

      struct tokenize_ctx *ctx = tokenize(line);

      if (dispatch(ctx) == -1) {
        printf("%s: command not found\n", line);
      }

      free_tokenize_ctx(ctx);
    }
    free(line);
  }

  free(line);
  return 0;
}
