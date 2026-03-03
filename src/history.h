#ifndef HISTORY_H
#define HISTORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 1024

char **read_full_history(int *out_n);
char *read_history();
void write_history(char *line);
int read_history_fd(char *path);
int write_history_fd(char *path, char *mode);
void load_env_history();
void persist_env_history();
void free_history();

//TODO: better encapsulate
extern int history_arrow_idx;
extern int history_idx;


#endif