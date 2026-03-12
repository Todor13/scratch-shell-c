#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>

#include "builtin.h"
#include "tokenizer.h"

char *read_line();

struct path_args {
    char path[1024];
    char prefix[1024];
};

#endif