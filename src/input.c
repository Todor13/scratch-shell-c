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

static int longest_common_prefix(char *prefix, char **candidates, int count)
{
  int res = -1;
  bool cond = true;
  int idx = strlen(prefix);
  while (cond) {
    char c = candidates[0][idx];
    for (int i = 1; i < count; i++) {
      if (c != candidates[i][idx]) {
        cond = false;
        break;
      }
    }
    prefix[idx++] = c;
    res = 0;
  }
  prefix[idx] = '\0';
  return res;
}

int autocomplete(char *buffer, int *len, int tab_count)
{
  int count = 0;
  char *candidates[1024];
  for (int i = 0; builtins[i].name != NULL; i++) {
    if (strncmp(buffer, builtins[i].name, *len) == 0) {
      candidates[count++] = strdup(builtins[i].name);
    }
  }

  char *executables[1024];
  int found_count = find_executables(buffer, executables, 1024);
  for (int i = 0; i < found_count; i++) {
    if (exists(candidates, count, executables[i]) == 0)
      candidates[count++] = executables[i];
  }

  int res = 0;
  if (count == 0) {
    res = -1;
  }

  if (count == 1) {
    *len = strlen(candidates[0]) + 1;
    snprintf(buffer, *len + 2, "%s ", candidates[0]);
  }

  if (count > 1 && tab_count > 0) {
    qsort(candidates, count, sizeof(char *), cmp_str);
    write(STDOUT_FILENO, "\n", 1);
    for (int i = 0; i < count; i++) {
      write(STDOUT_FILENO, candidates[i], strlen(candidates[i]));
      write(STDOUT_FILENO, "  ", 2);
    }
    write(STDOUT_FILENO, "\n", 1);
  }

  if (count > 1 && tab_count == 0 && longest_common_prefix(buffer, candidates, count) == -1) {
    res = -1;
  }

  for (int i = 0; i < count; i++) {
    free(candidates[i]);
  }

  return res;
}

char *read_line()
{
  enable_raw_mode();
  char *buffer = malloc(BUFFER_SIZE);
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
