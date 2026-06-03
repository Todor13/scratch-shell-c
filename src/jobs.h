#ifndef JOBS_H
#define JOBS_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "utils.h"

enum job_state {
    RUNNING,
    DONE,
    TERMINATED,
    STOPPED
};

struct job
{
  int id;
  char *cmd;
  pid_t pid;
  enum job_state state;
  int seq;
};

struct job **init_jobs();
void sigchld_handler(int sig);
void update_jobs();
void add_job(pid_t pid, char *name);
void print_jobs();
void print_job(struct job *j);
void reap_jobs();
void free_jobs();

#endif
