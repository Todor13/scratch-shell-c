#ifndef INPUT_H
#define INPUT_H

#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "builtin.h"

char *read_line();

#endif