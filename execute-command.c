// UCLA CS 111 Lab 1 command execution
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

#include "command.h"

int
command_status (command_t c)
{
  return c->status;
}

void
execute_sequence_command (command_t cmd, int tt)
{
  execute_command (cmd->u.command[0], tt);
  execute_command (cmd->u.command[1], tt);
  cmd->status = cmd->u.command[1]->status;
}

void
execute_and_command (command_t cmd, int tt)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c, tt);

      if (c->status != 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_or_command (command_t cmd, int tt)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c, tt);

      if (c->status == 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_pipe_command (command_t cmd, int tt)
{
  pid_t l_pid;
  pid_t r_pid;
  int pfd[2];
  int status;

  pipe (pfd);
  l_pid = fork ();
  if (l_pid == 0)
    {
      close (pfd[0]);
      dup2 (pfd[1], STDOUT_FILENO);
      close (pfd[1]);
      execute_command (cmd->u.command[0], tt);
      exit (cmd->u.command[0]->status);
    }
  else if (l_pid > 0)
    {
      r_pid = fork ();
      if (r_pid == 0)
        {
          close (pfd[1]);
          dup2 (pfd[0], STDIN_FILENO);
          close (pfd[0]);
          execute_command (cmd->u.command[1], tt);
          exit (cmd->u.command[1]->status);
        }
      else if (r_pid > 0)
        {
          close (pfd[0]);
          close (pfd[1]);
          waitpid (l_pid, &status, 0);
          waitpid (r_pid, &status, 0); // overwrite status
          cmd->status = status;
        }
    }
}

void
execute_simple_command (command_t cmd, int tt)
{
  tt; // silent compiler warnings
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      int input;
      int output;

      if (cmd->input != NULL)
        {
          input = open (cmd->input, O_RDONLY);
          dup2 (input, STDIN_FILENO);
          close (input);
        }

      if (cmd->output != NULL)
        {
          output = open (cmd->output, O_WRONLY | O_CREAT);
          dup2 (output, STDOUT_FILENO);
          close (output);
        }

      execvp (cmd->u.word[0], cmd->u.word);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = WEXITSTATUS (status);
    }
}

void
execute_subshell_command (command_t cmd, int tt)
{
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      int input;
      int output;

      if (cmd->input != NULL)
        {
          input = open (cmd->input, O_RDONLY);
          dup2 (input, STDIN_FILENO);
          close (input);
        }

      if (cmd->output != NULL)
        {
          output = open (cmd->output, O_WRONLY | O_CREAT);
          dup2 (output, STDOUT_FILENO);
          close (output);
        }

      execute_command (cmd->u.subshell_command, tt);
      exit (cmd->u.subshell_command->status);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = status;
    }
}

void
execute_command (command_t c, int tt)
{
  switch (c->type)
    {
      case SEQUENCE_COMMAND: execute_sequence_command (c, tt); break;
      case AND_COMMAND: execute_and_command (c, tt); break;
      case OR_COMMAND: execute_or_command (c, tt); break;
      case PIPE_COMMAND: execute_pipe_command (c, tt); break;
      case SIMPLE_COMMAND: execute_simple_command (c, tt); break;
      case SUBSHELL_COMMAND: execute_subshell_command (c, tt); break;
    }
}

