#include "exec.h"

static size_t init_size = 64;
static char **path_dirs = NULL;
static int count = 0;

void load_path_dirs()
{
  if (path_dirs != NULL)
    return;
  
  path_dirs = xmalloc(init_size * sizeof(char *));
  const char *path = getenv("PATH");
  if (!path)
    return;

  char *copy = strdup(path);
  if (!copy)
    return;

  char *saveptr = NULL;
  for (char *dir = strtok_r(copy, ":", &saveptr); dir; dir = strtok_r(NULL, ":", &saveptr)) {
    if (*dir == '\0')
      dir = ".";

    if (count == init_size) {
      init_size *= 2;
      path_dirs = xrealloc(path_dirs, init_size * sizeof(char *)); 
    }
    path_dirs[count++] = strdup(dir);
  }

  free(copy);
}

void free_path_dirs()
{
  for (int i = 0; i < count; i++)
  {
    free(path_dirs[i]);
  }
}

int exists(char **result, int count, char *cmd)
{
   for (int i = 0; i < count; i++) {
    if (strcmp(result[i], cmd) == 0)
      return 1;
   }
   
   return 0;
}

int find_executable(const char *cmd, char *out, size_t outsz)
{
  load_path_dirs();

  for (int i = 0; i < count; i++) {
    snprintf(out, outsz, "%s/%s", path_dirs[i], cmd);

    if (access(out, X_OK) == 0) {
      return 0;
    }
  }

  return -1;
}

int find_executables(const char *prefix, char **result, int max)
{
  load_path_dirs();

  int found_count = 0;
  for (int i = 0; i < count; i++) {
    DIR *dir = opendir(path_dirs[i]);
    if (!dir)
      continue;

    struct dirent *d;
    while ((d = readdir(dir))) {
      if (d->d_name[0] == '/')
        continue;

      if (strncmp(d->d_name, prefix, strlen(prefix)) != 0)
        continue;

      char full[1024];
      snprintf(full, sizeof(full), "%s/%s", path_dirs[i], d->d_name);

      if (found_count < max && exists(result, found_count, d->d_name) == 0 && access(full, X_OK) == 0)
        result[found_count++] = strdup(d->d_name);
    }
    closedir(dir);
  }

  return found_count;
}