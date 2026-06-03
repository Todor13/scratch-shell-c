#include "jobs.h"

struct job **jobs = NULL;
int jobs_len = 2;
int seq = 0;
volatile sig_atomic_t sigchld_reveived = 0;

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

static struct job **find_recent_jobs()
{
  struct job **result = xcalloc(2, sizeof(struct job *));
  int prev, max = -1;
  for (size_t i = 0; i < jobs_len; i++) {
    if (jobs[i] != NULL) {
      if (jobs[i]->seq > max) {
        if (result[0]) {
          prev = max;
          result[1] = result[0];
        }
        max = jobs[i]->seq;
        result[0] = jobs[i];
      }
    }
  }

  return result;
}

struct job **init_jobs()
{
  jobs = xcalloc(jobs_len, sizeof(struct job *));
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
  j->seq = seq++;
  jobs[idx] = j;

  printf("[%d] %d\n", idx + 1, pid);
}

void print_job(struct job *j)
{
  char current_sign = ' ';
  struct job **recent_jobs = find_recent_jobs();
  if (j == recent_jobs[0])
    current_sign = '+';
  if (j == recent_jobs[1])
    current_sign = '-';

  free(recent_jobs);

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

  char *ending = j->state == RUNNING ? " &" : "";
  printf("[%d]%c  %-17s  %s%s\n", j->id, current_sign, state_str, j->cmd, ending);
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

void reap_jobs()
{
  signal(SIGCHLD, sigchld_handler);
  update_jobs();
  for (int i = 0; i < jobs_len; i++) {
    if (jobs[i] != NULL) {
      if (jobs[i]->state != RUNNING) {
        print_job(jobs[i]);
        free_job(jobs[i]);
        jobs[i] = NULL;
      }
    }
  }
}

void free_jobs()
{
  for (int i = 0; i < jobs_len; i++) {
    if (jobs[i] != NULL)
      free_job(jobs[i]);
  }

  free(jobs);
}
