#include "jobs.h"

struct job **jobs = NULL;
int jobs_len = 2;
pid_t last_job_pid = -1;
pid_t prev_to_last_pid = -1;
volatile sig_atomic_t sigchld_reveived = 0;

struct job **init_jobs()
{
  jobs = calloc(jobs_len, sizeof(struct job *));
}

static int find_vacant()
{
  for (int i = 0; i < jobs_len; i++) {
    if (jobs[i] == NULL)
      return i;
  }

  int orig_size = jobs_len;
  jobs_len *= 2;
  jobs = xrealloc(jobs, jobs_len * sizeof(struct job *));
  memset(jobs + orig_size, 0, orig_size * sizeof(struct job *));
  return orig_size;
}

static void free_job(struct job *j)
{
  free(j->cmd);
  free(j);
}

void update_jobs()
{
  if (!sigchld_reveived)
    return;

  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    for (int i = 0; i < jobs_len; i++) {
      if (jobs[i] != NULL && jobs[i]->pid == pid && jobs[i]->state == RUNNING) {
        if (WIFEXITED(status)) {
          jobs[i]->state = DONE;
        } else if (WIFSIGNALED(status)) {
          jobs[i]->state = TERMINATED;
        } else if (WIFSTOPPED(status)) {
          jobs[i]->state = STOPPED;
        }
        print_job(jobs[i]);
        free_job(jobs[i]);
        jobs[i] = NULL;
        break;
      }
    }
  }
}

void sigchld_handler(int sig)
{
  (void)sig;
  sigchld_reveived = 1;
}

void add_job(pid_t pid, char *name)
{
  int idx = find_vacant();

  struct job *j = xmalloc(sizeof(struct job));
  j->id = idx + 1;
  j->cmd = name;
  j->pid = pid;
  j->state = RUNNING;
  jobs[idx] = j;

  if (last_job_pid > -1)
    prev_to_last_pid = last_job_pid;

  last_job_pid = pid;

  printf("[%d] %d\n", idx + 1, pid);
}

void print_job(struct job *j)
{
  char current_sign = ' ';
  if (j->pid == last_job_pid)
    current_sign = '+';
  else if (j->pid == prev_to_last_pid)
    current_sign = '-';

  char *state_str;

  switch (j->state) {
  case RUNNING:
    state_str = "Running";
    break;
  case STOPPED:
    state_str = "Stopped";
    break;
  case DONE:
    state_str = "Done";
    break;
  case TERMINATED:
    state_str = "Terminated";
    break;
  }

  printf("[%d]%c  %-10s  %s\n", j->id, current_sign, state_str, j->cmd);
}

void print_jobs()
{
  signal(SIGCHLD, sigchld_handler);
  update_jobs();
  for (int i = 0; i < jobs_len; i++) {
    if (jobs[i] != NULL) {
      print_job(jobs[i]);
      if (jobs[i]->state != RUNNING) {
        free_job(jobs[i]);
        jobs[i] = NULL;
      }
    }
  }
}