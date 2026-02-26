#include "tokenizer.h"

static int single_quote_mask = 0x01;
static int double_quote_mask = 0x02;
static int escape_mask = 0x04;

static void write_buffer(struct tokenize_result *result, char *buf, int *blen)
{
  buf[*blen] = '\0';
  if (result->redirect == REDIRECT_STDOUT || result->redirect == REDIRECT_STDERR) {
    result->redirect_path = malloc(*blen + 1);
    strcpy(result->redirect_path, buf);
  } else {
    result->argv[result->argc] = malloc(*blen + 1);
    strcpy(result->argv[result->argc++], buf);
  }
  *blen = 0;
}

static void flush_buffer_pipe(struct tokenize_result *result, char *buf, int *blen)
{
  result->pipe_args[result->n_pipes].argc = result->argc;
  for (int i = 0; i < result->argc; i++) {
    result->pipe_args[result->n_pipes].argv[i] = result->argv[i];
  }
  result->argc = 0;
  result->n_pipes++;
}

struct tokenize_result *tokenize(char *input)
{
  struct tokenize_result *result = malloc(sizeof(struct tokenize_result));
  result->argc = 0;
  result->redirect = REDIRECT_NONE;
  result->redirect_path = NULL;
  result->n_pipes = 0;
  result->prev_read = -1;
  result->pipefd[0] = -1;
  result->pipefd[1] = -1;
  char buf[1024];
  int mode = 0;
  int blen = 0;
  int i = 0;
  for (;;) {
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
      result->redirect = REDIRECT_PIPE;
      flush_buffer_pipe(result, buf, &blen);
      continue;
    }

    bool term = c == '\0';
    if (mode == 0 && isspace(c) > 0 || term) {
      if (term) {
        write_buffer(result, buf, &blen);
        if (result->redirect == REDIRECT_PIPE)
          flush_buffer_pipe(result, buf, &blen);
        break;
      }

      if (blen == 0)
        continue;

      write_buffer(result, buf, &blen);
      continue;
    }

    if (c == '\\' && !(mode & (escape_mask | single_quote_mask))) {
      mode ^= escape_mask;
      continue;
    }

    if (mode == 0) {
      if ((c == '1' || c == '2') && input[i] == '>') {
        result->redirect = (c == '1' ? REDIRECT_STDOUT : REDIRECT_STDERR);
        result->redirect_mode = OVERRIDE;
        i++;
        if (input[i] == '>') {
          i++;
          result->redirect_mode = APPPEND;
        }
      }
      if (c == '>') {
        result->redirect = REDIRECT_STDOUT;
        result->redirect_mode = OVERRIDE;
        if (input[i] == '>') {
          i++;
          result->redirect_mode = APPPEND;
        }
      }
    }

    buf[blen++] = c;

    if (mode & escape_mask)
      mode ^= escape_mask;
  }

  result->argv[result->argc] = NULL;
  return result;
}

void free_tokenize_result(struct tokenize_result *result)
{
  if (result->n_pipes != 0) {
    for (int i = 0; i < result->n_pipes; i++) {
      for (int j = 0; j < result->pipe_args[i].argc; j++) 
        free(result->pipe_args[i].argv[j]);
    }
  } else {
    for (int i = 0; i < result->argc; i++) {
      free(result->argv[i]);
    }

    if (result->redirect_path != NULL) {
      free(result->redirect_path);
    }
  }

  free(result);
}