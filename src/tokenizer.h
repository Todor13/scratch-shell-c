#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define N_PIPES 64
#define N_ARGV 64

enum redirect {
    REDIRECT_PIPE, REDIRECT_NONE, REDIRECT_STDOUT, REDIRECT_STDERR
};

enum redirect_mode {
    OVERRIDE, APPPEND
};

struct pipe_args {
    int argc;
    char *argv[N_ARGV];
};

struct tokenize_result {
    int argc;
    char *argv[N_ARGV];
    enum redirect redirect;
    enum redirect_mode redirect_mode;
    char *redirect_path;
    int n_pipes;
    struct pipe_args pipe_args[N_PIPES];
    int pipefd[2];
    int current_pipe;
    int prev_read;
};

struct tokenize_result *tokenize(char *input);
void destruct_result(struct tokenize_result *result);

#endif
