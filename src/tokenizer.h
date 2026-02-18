#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

enum redirect {
    NONE, STDOUT, STDERR
};

enum redirect_mode {
    OVERRIDE, APPPEND
};

struct tokenize_result {
    int argc;
    char *argv[100];
    enum redirect redirect;
    enum redirect_mode redirect_mode;
    char *redirect_path;
};

struct tokenize_result *tokenize(char *input);
void destruct_result(struct tokenize_result *result);

#endif
