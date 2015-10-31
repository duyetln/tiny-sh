#include <stdlib.h>
#include <string.h>
#include <error.h>

#include "concurrent_commands.h"

struct dependency
{
  int index;
  int count;

  struct command *cmd;
  struct dependency **waiters;
  struct dependency **blockers;
  char **reads;
  char **writes;

};

typedef struct dependency *dependency_t;

#define malloc_pointer(type, count) ((type *) malloc ((count) * sizeof (type)))

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
      tmp = malloc_pointer (char *, count + 1);
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
              second = malloc_pointer (char *, 2);
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
              second = malloc_pointer (char *, 2);
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

int
intersect (char **first, char **second)
{
  char **it1;
  char **it2;

  it1 = first;
  while (it1 && *it1)
    {
      it2 = second;
      while (it2 && *it2)
        {
          if (strcmp (*it1, *it2) == 0)
            return 1;
          it2++;
        }
      it1++;
    }

  return 0;
}

dependency_t *
create_dep_ptr_arr (int count)
{
  dependency_t *arr = malloc_pointer (dependency_t, count);
  int it = 0;
  while (it < count)
    arr[it++] = NULL;
  return arr;
}

dependency_t
create_dependency (command_t cmd, int index, int count)
{
  dependency_t dep = malloc_pointer (struct dependency, 1);
  dep->index = index;
  dep->count = count;
  dep->cmd = cmd;
  dep->reads = get_reads (cmd);
  dep->writes = get_writes (cmd);
  dep->waiters = create_dep_ptr_arr (count);
  dep->blockers = create_dep_ptr_arr (count);
  return dep;
}

void
destroy_dependency (dependency_t dep)
{
  if (dep->reads)
    free (dep->reads);
  if (dep->writes)
    free (dep->writes);
  free (dep->waiters);
  free (dep->blockers);
  free (dep);
}

dependency_t *
create_dependencies (command_stream_t strm)
{
  int count;
  int index;
  dependency_t *deps;
  dependency_t *it1;
  dependency_t *it2;
  command_t cmd;

  count = 0;
  while (current_command (strm))
    {
      count++;
      next_command (strm);
    }
  reset_command_stream (strm);

  deps = malloc_pointer (dependency_t, count + 1);
  deps[count] = NULL;

  index = 0;
  while ((cmd = current_command (strm)))
    {
      deps[index] = create_dependency (cmd, index, count);
      index++;
      next_command (strm);
    }

  it1 = deps;
  while (it1 && *it1)
    {
      it2 = deps;
      while (it2 && it2 && *it2 != *it1)
        {
          dependency_t dep1 = *it1;
          dependency_t dep2 = *it2;
          if (intersect (dep2->writes, dep1->reads) || // Read-After-Write
            intersect (dep2->reads, dep1->writes) || // Write-After-Read
            intersect (dep2->writes, dep1->writes)) // Write-After-Write
            {
              dep2->waiters[dep1->index] = dep1;
              dep1->blockers[dep2->index] = dep2;
            }
          it2++;
        }
      it1++;
    }

  return deps;
}

void
destroy_dependencies (dependency_t *deps)
{
  dependency_t *tmp = deps;
  while (*tmp)
  {
    destroy_dependency (*tmp);
    tmp++;
  }
  free (deps);
}
