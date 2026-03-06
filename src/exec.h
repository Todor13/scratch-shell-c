#ifndef EXEC_H
#define EXEC_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

int find_executable(const char *cmd, char *out, size_t outsz);
int find_executables(const char *prefix, char **result, int max);
int exists(char **result, int count, char *cmd);

#endif