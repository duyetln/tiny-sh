#include <stdlib.h>
#include <string.h>
#include <error.h>

#include "concurrent_commands.h"

struct dependency
{
  int index;

  struct command *cmd;
  char **reads;
  char **writes;
  int *waiters;
  int *blockers;
};

#define TRUE 1
#define FALSE 0

typedef struct dependency *dependency_t;

char **
merge (char **first, char **second)
{
  int count;
  char **tmp;

  count = 0;

  tmp = first;
  while (*tmp++)
    count++;

  tmp = second;
  while (*tmp++)
    count++;

  if (count > 0)
    {
      tmp = (char **) malloc ((count + 1) * sizeof (char *));
      tmp[count] = NULL;
      while (*first)
        *tmp++ = *first++;
      while (*second)
        *tmp++ = *second++;
      tmp = tmp - count;
      return tmp;
    }
  else
    return NULL;
}

char **
get_reads (command_t cmd)
{
  cmd;
  return NULL;
}

char **
get_writes (command_t cmd)
{
  cmd;
  return NULL;
}

dependency_t *
create_dependencies (command_stream_t strm)
{
  int count;
  int index;
  dependency_t *dep;
  command_t cmd;

  count = 0;
  while (current_command (strm))
    {
      count++;
      next_command (strm);
    }
  reset_command_stream (strm);

  dep = (dependency_t *) malloc ((count + 1) * sizeof (dependency_t));
  dep[count] = NULL;

  index = 0;
  while ((cmd = current_command (strm)))
    {
      dependency_t cdep = (dependency_t) malloc (sizeof (struct dependency));
      cdep->index = index;
      cdep->cmd = cmd;
      cdep->reads = get_reads (cmd);
      cdep->writes = get_writes (cmd);

      dep[index] = cdep;

      index++;
      next_command (strm);
    }

  return dep;
}

void
destroy_dependencies (dependency_t *dep)
{
  dependency_t *tmp = dep;
  while (tmp)
  {
    free ((*tmp)->reads);
    free ((*tmp)->writes);
    free ((*tmp)->waiters);
    free ((*tmp)->blockers);
    free ((*tmp));
    tmp++;
  }
  free (dep);
}
