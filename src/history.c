#include "history.h"

static size_t init_size = 2;
char **history = NULL;
int history_arrow_idx = 0;
int history_idx = 0;
int append_idx = 0;

char *read_history()
{
  return history[history_arrow_idx];
}

char **read_full_history(int *out_n)
{
  *out_n = history_idx;
  return history;
}

void write_history(char *line)
{
  if (history_idx == init_size) {
    init_size *= 2;
    history = xrealloc(history, init_size * sizeof(char *));
  }

  history[history_idx++] = line;
}

int read_history_fd(char *path)
{
  FILE *fp = fopen(path, "r");
  if (!fp)
    return 1;

  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), fp)) {
    buffer[strcspn(buffer, "\n")] = '\0';
    write_history(strdup(buffer));
  }

  fclose(fp);
  return 0;
}

int write_history_fd(char *path, char *mode)
{
  FILE *fp = fopen(path, mode);
  if (!fp)
    return 1;
  int i = 0;
  if (mode[0] == 'a') {
    i = append_idx;
    append_idx = history_idx;
  }

  for (i; i < history_idx; i++) {
    fprintf(fp, "%s\n", history[i]);
  }

  fclose(fp);
  return 0;
}

void load_env_history()
{
  history = xmalloc(init_size * sizeof(char *));

  char *histfile_env = getenv("HISTFILE");
  if (histfile_env != NULL) {
    read_history_fd(histfile_env);
  }
}

void persist_env_history()
{
  char *histfile_env = getenv("HISTFILE");
  if (histfile_env != NULL) {
    write_history_fd(histfile_env, "w");
  }
}

void free_history()
{
  for (int i = 0; i < history_idx; i++) {
    free(history[i]);
  }
}