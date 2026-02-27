#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_HISTORY 1024

char *read_line();
char **read_history(int *out_n);
void write_history(char *line);
int read_history_fd(char *path);
int write_history_fd(char *path, char *mode);
void load_env_history();
void free_history();

#endif