#include "input.h"

const int BUFFER_SIZE = 1024;
static struct termios default_termios;

static void enable_raw_mode()
{
  tcgetattr(STDIN_FILENO, &default_termios);
  struct termios raw_termios = default_termios;
  raw_termios.c_lflag &= ~(ICANON | ECHO);
  raw_termios.c_cc[VMIN] = 1;
  raw_termios.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
}

static void disable_raw_mode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &default_termios);
}

static void redraw_line(const char *buf)
{
  write(STDOUT_FILENO, "\r", 1);
  write(STDOUT_FILENO, "\033[2K", 4);
  write(STDOUT_FILENO, "$ ", 2);
  write(STDOUT_FILENO, buf, strlen(buf));
}

static void redraw_from_history(char *buffer, int *len)
{
  char *record = read_history();
  strcpy(buffer, record);
  *len = strlen(record);
  redraw_line(record);
}

static int cmp_str(const void *a, const void *b)
{
  return strcmp(*(char *const *)a, *(char *const *)b);
}

static char *longest_common_prefix(char *prefix, char **candidates, int count)
{
  char *common_prefix = NULL;
  int orig_len = strlen(prefix);
  int idx = orig_len;
  bool cond = true;
  while (cond) {
    char c = candidates[0][idx];
    for (int i = 1; i < count; i++) {
      if (c != candidates[i][idx]) {
        cond = false;
        break;
      }
    }
    if (cond)
      idx++;
  }

  if (idx - orig_len > 0) {
    common_prefix = xmalloc((idx + 1) * sizeof(char));
    strncpy(common_prefix, candidates[0], idx);
    common_prefix[idx] = '\0';
  }

  return common_prefix;
}

static void show_matches(char **candidates, int count)
{
  qsort(candidates, count, sizeof(char *), cmp_str);
  write(STDOUT_FILENO, "\n", 1);
  for (int i = 0; i < count; i++) {
    write(STDOUT_FILENO, candidates[i], strlen(candidates[i]));
    write(STDOUT_FILENO, "  ", 2);
  }
  write(STDOUT_FILENO, "\n", 1);
}

static struct path_args *split_path_prefix(const char *arg, int *out_len)
{
  struct path_args *p = xmalloc(sizeof(struct path_args));
  char *pos = strrchr(arg, '/');
  if (pos != NULL) {
    int idx = (pos - arg + 1);
    strncpy(p->path, arg, idx);
    p->path[idx] = '\0';
    strcpy(p->prefix, (pos + 1));
    *out_len = idx;
  } else {
    p->path[0] = '.';
    p->path[1] = '\0';
    strcpy(p->prefix, arg);
    *out_len = 0;
  }

  return p;
}

static void expand_buffer(char *buffer, int *len, char *suffix, int pos, bool autocomplete)
{
  char *dest = buffer + pos;
  strcpy(dest, suffix);
  *len = strlen(buffer);
  if (buffer[*len - 1] != '/' && !autocomplete) {
    buffer[*len] = ' ';
    buffer[*len + 1] = '\0';
    *len = *len + 1;
  }
}

static int autocomplete(char *buffer, int *len, int tab_count)
{
  struct tokenize_ctx *ctx = tokenize(buffer);
  int res = 0;
  char *arg = xstrdup(ctx->argv[ctx->argc - 1]);
  int new_buf_pos = strlen(buffer) - strlen(arg);
  int c_count = 0;
  char *candidates[1024];
  int path_len = 0;

  if (ctx->argc == 1) {
    for (int i = 0; builtins[i].name != NULL; i++) {
      if (strncmp(arg, builtins[i].name, *len) == 0) {
        candidates[c_count++] = xstrdup(builtins[i].name);
      }
    }

    char *executables[1024];
    int found_count = find_executables(arg, executables, 1024);
    for (int i = 0; i < found_count; i++) {
      if (exists(candidates, c_count, executables[i]) == 0)
        candidates[c_count++] = executables[i];
    }
  }

  if (ctx->argc > 1) {
    struct path_args *p = split_path_prefix(arg, &path_len);
    DIR *dir = opendir(p->path);
    if (dir) {
      struct dirent *e;

      while ((e = readdir(dir))) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
          continue;

        if (strncmp(e->d_name, p->prefix, strlen(p->prefix)) == 0) {
          struct stat st;
          char full[1024];
          snprintf(full,
                   path_len + strlen(p->prefix) + strlen(e->d_name) + 3,
                   "%s/%s",
                   p->path,
                   e->d_name);
          if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
              char cand[258];
              snprintf(cand, strlen(e->d_name) + 2, "%s/", e->d_name);
              candidates[c_count++] = xstrdup(cand);
            } else
              candidates[c_count++] = xstrdup(e->d_name);
          }
        }
      }
      closedir(dir);
    }
    free(p);
  }

  if (c_count == 0) {
    res = -1;
  }

  if (c_count == 1) {
    expand_buffer(buffer, len, candidates[0], new_buf_pos + path_len, false);
  }

  if (c_count > 1 && tab_count > 0) {
    show_matches(candidates, c_count);
  }

  if (c_count > 1 && tab_count == 0) {
    char *common_prefix = longest_common_prefix(arg + path_len, candidates, c_count);
    if (common_prefix != NULL) {
      expand_buffer(buffer, len, common_prefix, new_buf_pos + path_len, true);
      free(common_prefix);
    } else {
      res = -1;
    }
  }

  for (int i = 0; i < c_count; i++) {
    free(candidates[i]);
  }
  free_tokenize_ctx(ctx);
  free(arg);

  return res;
}

char *read_line()
{
  enable_raw_mode();
  char *buffer = xmalloc(BUFFER_SIZE * sizeof(char *));
  int len = 0;
  buffer[0] = '\0';
  write(STDOUT_FILENO, "$ ", 2);
  int tab_count = 0;

  for (;;) {
    char c;
    read(STDIN_FILENO, &c, 1);

    if (c == '\t') {
      if (autocomplete(buffer, &len, tab_count++) == -1) {
        write(STDOUT_FILENO, "\x07", 1);
      }

      redraw_line(buffer);
      continue;
    }

    if (c == '\n' || c == '\r') {
      buffer[len] = '\0';
      write(STDOUT_FILENO, "\n", 1);
      history_arrow_idx = 0;
      break;
    } else if (c == 27) {
      char seq[2];
      read(STDIN_FILENO, &seq[0], 1);
      read(STDIN_FILENO, &seq[1], 1);

      if (seq[0] == '[') {
        if (seq[1] == 'A') {
          // up
          if (history_idx > 0) {
            if (history_arrow_idx - 1 < 0) {
              history_arrow_idx = history_idx - 1;
            } else {
              history_arrow_idx--;
            }

            redraw_from_history(buffer, &len);
          }
        } else if (seq[1] == 'B') {
          // down
          if (history_idx > 0) {
            if (history_arrow_idx + 1 > history_idx - 1) {
              history_arrow_idx = 0;
            } else {
              history_arrow_idx++;
            }

            redraw_from_history(buffer, &len);
          }
        } else if (seq[1] == 'C') {
          // right
        } else if (seq[1] == 'D') {
          // left
        }
      }
    } else if (c == 127 || c == 8) {
      if (len > 0) {
        len--;
        buffer[len] = '\0';
        redraw_line(buffer);
      }
    } else {
      if (len < BUFFER_SIZE - 1) {
        buffer[len++] = c;
        buffer[len] = '\0';
        write(STDOUT_FILENO, &c, 1);
      }
    }
    tab_count = 0;
  }

  disable_raw_mode();
  return buffer;
}
