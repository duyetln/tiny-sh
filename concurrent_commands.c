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
  while (tmp && *tmp++)
    count++;

  tmp = second;
  while (tmp && *tmp++)
    count++;

  if (count > 0)
    {
      tmp = (char **) malloc ((count + 1) * sizeof (char *));
      tmp[count] = NULL;
      while (first && *first)
        *tmp++ = *first++;
      while (second && *second)
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
  char **result = NULL;
  char **first = NULL;
  char **second = NULL;

  switch (cmd->type)
    {
      case SEQUENCE_COMMAND:
      case AND_COMMAND:
      case OR_COMMAND:
      case PIPE_COMMAND:
        {
          first = get_reads (cmd->u.command[0]);
          second = get_reads (cmd->u.command[1]);
          result =  merge (first, second);
          break;
        }
      case SUBSHELL_COMMAND:
        {
          result = get_reads (cmd->u.subshell_command);
          break;
        }
      case SIMPLE_COMMAND:
        {
          result = merge (cmd->u.word + 1, NULL);
          break;
        }
   }

  switch (cmd->type)
    {
      case SIMPLE_COMMAND:
      case SUBSHELL_COMMAND:
        {
          if (cmd->input)
            {
              first = result;
              second = (char **) malloc (2 * sizeof (char *));
              second[1] = NULL;
              second[0] = cmd->input;
              result = merge (first, second);
            }
          break;
        }
      default:
        ;
    }

  if (first)
    free (first);
  if (second)
    free (second);

  return result;
}

char **
get_writes (command_t cmd)
{
  char **result = NULL;
  char **first = NULL;
  char **second = NULL;
  switch (cmd->type)
    {
      case SEQUENCE_COMMAND:
      case AND_COMMAND:
      case OR_COMMAND:
      case PIPE_COMMAND:
        {
          first = get_writes (cmd->u.command[0]);
          second = get_writes (cmd->u.command[1]);
          result =  merge (first, second);
          break;
        }
      case SUBSHELL_COMMAND:
        {
          result = get_writes (cmd->u.subshell_command);
          break;
        }
      case SIMPLE_COMMAND:
        ;
   }

  switch (cmd->type)
    {
      case SIMPLE_COMMAND:
      case SUBSHELL_COMMAND:
        {
          if (cmd->output)
            {
              first = result;
              second = (char **) malloc (2 * sizeof (char *));
              second[1] = NULL;
              second[0] = cmd->output;
              result = merge (first, second);
            }
          break;
         }
      default:
        ;
    }

  if (first)
    free (first);
  if (second)
    free (second);

  return result;
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
