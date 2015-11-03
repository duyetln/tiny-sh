// UCLA CS 111 Lab 1 command execution
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "command_utility.h"

static void
command_indented_print (int indent, command_t c)
{
  switch (c->type)
    {
    case CMD_AND:
    case CMD_SEQUENCE:
    case CMD_OR:
    case CMD_PIPE:
      {
        command_indented_print (indent + 2 * (c->u.command[0]->type != c->type), c->u.command[0]);
        static char const command_label[][3] = { "&&", ";", "||", "|" };
        printf (" \\\n%*s%s\n", indent, "", command_label[c->type]);
        command_indented_print (indent + 2 * (c->u.command[1]->type != c->type), c->u.command[1]);
        break;
      }
    case CMD_SIMPLE:
      {
        char **w = c->u.word;
        printf ("%*s%s", indent, "", *w);
        while (*++w)
          printf (" %s", *w);
        break;
      }
    case CMD_SUBSHELL:
      {
        printf ("%*s(\n", indent, "");
        command_indented_print (indent + 1, c->u.subshell_command);
        printf ("\n%*s)", indent, "");
        break;
      }
    default:
      abort ();
    }

  if (c->input)
    printf ("<%s", c->input);
  if (c->output)
    printf (">%s", c->output);
}

void
print_command (command_t c)
{
  command_indented_print (2, c);
  putchar ('\n');
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_sequence_command (command_t cmd)
{
  execute_command (cmd->u.command[0]);
  execute_command (cmd->u.command[1]);
  cmd->status = cmd->u.command[1]->status;
}

void
execute_and_command (command_t cmd)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c);

      if (c->status != 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_or_command (command_t cmd)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c);

      if (c->status == 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_pipe_command (command_t cmd)
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
      execute_command (cmd->u.command[0]);
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
          execute_command (cmd->u.command[1]);
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

int
open_input_redirection (command_t cmd)
{
  int input = open (cmd->input, O_RDONLY);
  if (input != -1)
    {
      dup2 (input, STDIN_FILENO);
      close (input);
    }

  return input;
}

int
open_output_redirection (command_t cmd)
{
  int output = open (cmd->output, O_WRONLY | O_CREAT);
  if (output != -1)
    {
      dup2 (output, STDOUT_FILENO);
      close (output);
    }

  return output;
}

void
execute_simple_command (command_t cmd)
{
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      if (cmd->input != NULL && open_input_redirection (cmd) == -1)
        error (errno, errno, "%s", cmd->input);

      if (cmd->output != NULL && open_output_redirection (cmd) == -1)
        error (errno, errno, "%s", cmd->output);

      execvp (cmd->u.word[0], cmd->u.word);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = WEXITSTATUS (status);
    }
}

void
execute_subshell_command (command_t cmd)
{
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      if (cmd->input != NULL && open_input_redirection (cmd) == -1)
        error (errno, errno, "%s", cmd->input);

      if (cmd->output != NULL && open_output_redirection (cmd) == -1)
        error (errno, errno, "%s", cmd->output);

      execute_command (cmd->u.subshell_command);
      exit (cmd->u.subshell_command->status);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = status;
    }
}

void
execute_command (command_t c)
{
  switch (c->type)
    {
      case CMD_SEQUENCE: execute_sequence_command (c); break;
      case CMD_AND: execute_and_command (c); break;
      case CMD_OR: execute_or_command (c); break;
      case CMD_PIPE: execute_pipe_command (c); break;
      case CMD_SIMPLE: execute_simple_command (c); break;
      case CMD_SUBSHELL: execute_subshell_command (c); break;
    }
}

