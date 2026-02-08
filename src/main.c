#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  char *line = NULL;
  size_t len = 0;
  __ssize_t nread;

  while (1)
  {
    printf("$ ");
    nread = getline(&line, &len, stdin);

    if (nread == -1)
    {
      perror("getline");
      free(line);
      return 1;
    }

    if (nread > 0 && line[nread - 1] == '\n')
      line[nread - 1] = '\0';

    printf("%s: command not found\n", line);
  }

  free(line);
  return 0;
}
