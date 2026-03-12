#include "tokenizer.h"

static int single_quote_mask = 0x01;
static int double_quote_mask = 0x02;
static int escape_mask = 0x04;

static struct tokenize_ctx *init_ctx()
{
  struct tokenize_ctx *ctx = malloc(sizeof(struct tokenize_ctx));
  ctx->argc = 0;
  ctx->redirect = REDIRECT_NONE;
  ctx->redirect_path = NULL;
  ctx->n_pipes = 0;
  ctx->prev_read = -1;
  ctx->pipefd[0] = -1;
  ctx->pipefd[1] = -1;
  ctx->total_len = 0;
  return ctx;
}

static void write_buffer(struct tokenize_ctx *ctx, char *buf, int *blen)
{
  buf[*blen] = '\0';
  if (ctx->redirect == REDIRECT_STDOUT || ctx->redirect == REDIRECT_STDERR) {
    ctx->redirect_path = malloc(*blen + 1);
    strcpy(ctx->redirect_path, buf);
  } else {
    ctx->argv[ctx->argc] = malloc(*blen + 1);
    strcpy(ctx->argv[ctx->argc++], buf);
  }
  *blen = 0;
}

static void flush_buffer_pipe(struct tokenize_ctx *ctx, char *buf, int *blen)
{
  ctx->pipe_args[ctx->n_pipes].argc = ctx->argc;
  for (int i = 0; i < ctx->argc; i++) {
    ctx->pipe_args[ctx->n_pipes].argv[i] = ctx->argv[i];
  }
  ctx->argc = 0;
  ctx->n_pipes++;
}

struct tokenize_ctx *tokenize(char *input)
{
  struct tokenize_ctx *ctx = init_ctx();
  char buf[1024];
  int mode = 0;
  int blen = 0;
  int i = 0;
  for (;;) {
    ctx->total_len++;
    char c = input[i++];

    if (c == '\"' && !(mode & (single_quote_mask | escape_mask))) {
      mode ^= double_quote_mask;
      continue;
    }

    if (c == '\'' && !(mode & (double_quote_mask | escape_mask))) {
      mode ^= single_quote_mask;
      continue;
    }

    if (mode == 0 && c == '|') {
      ctx->redirect = REDIRECT_PIPE;
      flush_buffer_pipe(ctx, buf, &blen);
      continue;
    }

    bool term = c == '\0';
    if (mode == 0 && isspace(c) > 0 || term) {
      if (term) {
        write_buffer(ctx, buf, &blen);
        if (ctx->redirect == REDIRECT_PIPE)
          flush_buffer_pipe(ctx, buf, &blen);
        break;
      }

      if (blen == 0)
        continue;

      write_buffer(ctx, buf, &blen);
      continue;
    }

    if (c == '\\' && !(mode & (escape_mask | single_quote_mask))) {
      mode ^= escape_mask;
      continue;
    }

    if (mode == 0) {
      if ((c == '1' || c == '2') && input[i] == '>') {
        ctx->redirect = (c == '1' ? REDIRECT_STDOUT : REDIRECT_STDERR);
        ctx->redirect_mode = OVERRIDE;
        i++;
        if (input[i] == '>') {
          i++;
          ctx->redirect_mode = APPPEND;
        }
      }
      if (c == '>') {
        ctx->redirect = REDIRECT_STDOUT;
        ctx->redirect_mode = OVERRIDE;
        if (input[i] == '>') {
          i++;
          ctx->redirect_mode = APPPEND;
        }
      }
    }

    buf[blen++] = c;

    if (mode & escape_mask)
      mode ^= escape_mask;
  }

  ctx->argv[ctx->argc] = NULL;
  return ctx;
}

void free_tokenize_ctx(struct tokenize_ctx *ctx)
{
  if (ctx->n_pipes != 0) {
    for (int i = 0; i < ctx->n_pipes; i++) {
      for (int j = 0; j < ctx->pipe_args[i].argc; j++)
        free(ctx->pipe_args[i].argv[j]);
    }
  } else {
    for (int i = 0; i < ctx->argc; i++) {
      free(ctx->argv[i]);
    }

    if (ctx->redirect_path != NULL) {
      free(ctx->redirect_path);
    }
  }

  free(ctx);
}