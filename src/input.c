#include "input.h"

char *history[MAX_HISTORY];
int history_idx = 0;
int history_arrow_idx = 0;
int append_idx = 0;

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

char **read_history(int *out_n)
{
  *out_n = history_idx;
  return history;
}

void write_history(char *line)
{
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
  char *histfile_env = getenv("HISTFILE");
  if (histfile_env != NULL) {
    read_history_fd(histfile_env);
  }
}

char *read_line()
{
  enable_raw_mode();
  char *buffer = malloc(BUFFER_SIZE);
  int len = 0;
  buffer[0] = '\0';
  write(STDOUT_FILENO, "$ ", 2);

  for (;;) {
    char c;
    read(STDIN_FILENO, &c, 1);

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

            char *record = history[history_arrow_idx];
            strcpy(buffer, record);
            len = strlen(record);
            redraw_line(record);
          }
        } else if (seq[1] == 'B') {
          // down
          if (history_idx > 0) {
            if (history_arrow_idx + 1 > history_idx - 1) {
              history_arrow_idx = 0;
            } else {
              history_arrow_idx++;
            }

            char *record = history[history_arrow_idx];
            strcpy(buffer, record);
            len = strlen(record);
            redraw_line(record);
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
  }

  disable_raw_mode();
  return buffer;
}

void free_history()
{
  for (int i = 0; i < history_idx; i++) {
    free(history[i]);
  }
}
