#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "command_utility.h"

#define io_num(io, d) (io->io_num != NULL ? (int) strtol (io->io_num, NULL, 10) : (d))

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

  io_node_t io = c->io_head;
  while (io != NULL)
    {
      printf (" ");
      if (io->io_num)
        printf ("%s", io->io_num);

      switch (io->op)
        {
          case IO_INPUT: printf ("<%s", io->word); break;
          case IO_DUPIN: printf ("<&%s", io->word); break;
          case IO_IODUAL: printf ("<>%s", io->word); break;
          case IO_OUTPUT: printf (">%s", io->word); break;
          case IO_DUPOUT: printf (">&%s", io->word); break;
          case IO_APPEND: printf (">>%s", io->word); break;
          case IO_CLOBBER: printf (">|%s", io->word); break;
        }

      io = io->next;
    }
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
execute_sequence_command (command_t cmd, int no_clobber)
{
  execute_command (cmd->u.command[0], no_clobber);
  execute_command (cmd->u.command[1], no_clobber);
  cmd->status = cmd->u.command[1]->status;
}

void
execute_and_command (command_t cmd, int no_clobber)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c, no_clobber);

      if (c->status != 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_or_command (command_t cmd, int no_clobber)
{
  int i;
  command_t c = NULL;
  for (i = 0; i < 2; i++)
    {
      c = cmd->u.command[i];
      execute_command (c, no_clobber);

      if (c->status == 0)
        break;
    }
  cmd->status = c->status;
}

void
execute_pipe_command (command_t cmd, int no_clobber)
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
      execute_command (cmd->u.command[0], no_clobber);
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
          execute_command (cmd->u.command[1], no_clobber);
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

void open_io (command_t cmd, int no_clobber)
{
  int fdionum;
  int fdword;
  int closefdword = 1;
  int write_flags =  O_WRONLY | O_CREAT | O_TRUNC;
  int append_flags = O_WRONLY | O_CREAT | O_APPEND;
  int rw_flags = O_RDWR | O_CREAT;
  int perm_flags = S_IRUSR | S_IWUSR;
  io_node_t io = cmd->io_head;

  io = cmd->io_head;
  while (io != NULL)
    {
      if (io->op == IO_INPUT)
        {
          fdionum = io_num (io, STDIN_FILENO);
          fdword = open (io->word, O_RDONLY);
        }
      else if (io->op == IO_DUPIN)
        {
          fdionum = io_num (io, STDIN_FILENO);
          fdword = (int) strtol (io->word, NULL, 10);
          closefdword = 0;
        }
      else if (io->op == IO_IODUAL)
        {
          fdionum = io_num (io, STDIN_FILENO);
          fdword = open (io->word, rw_flags, perm_flags);
        }
      else if (io->op == IO_OUTPUT)
        {
          if (no_clobber && access (io->word, F_OK) != -1)
            error (errno, errno, "overwriting existing file");
          fdionum = io_num (io, STDOUT_FILENO);
          fdword = open (io->word, write_flags, perm_flags);
        }
      else if (io->op == IO_DUPOUT)
        {
          fdionum = io_num (io, STDOUT_FILENO);
          fdword = (int) strtol (io->word, NULL, 10);
          closefdword = 0;
        }
      else if (io->op == IO_APPEND)
        {
          fdionum = io_num (io, STDOUT_FILENO);
          fdword = open (io->word, append_flags, perm_flags);
        }
      else if (io->op == IO_CLOBBER)
        {
          fdionum = io_num (io, STDOUT_FILENO);
          fdword = open (io->word, write_flags, perm_flags);
        }

      if (fdword == -1 || dup2 (fdword, fdionum) == -1)
        error (errno, errno, NULL);
      if (closefdword)
        close (fdword);

      io = io->next;
    }
}

void
execute_simple_command (command_t cmd, int no_clobber)
{
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      open_io (cmd, no_clobber);
      execvp (cmd->u.word[0], cmd->u.word);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = WEXITSTATUS (status);
    }
}

void
execute_subshell_command (command_t cmd, int no_clobber)
{
  pid_t child_pid;
  int status;

  child_pid = fork ();
  if (child_pid == 0)
    {
      open_io (cmd, no_clobber);
      execute_command (cmd->u.subshell_command, no_clobber);
      exit (cmd->u.subshell_command->status);
    }
  else if (child_pid > 0)
    {
      waitpid (child_pid, &status, 0);
      cmd->status = status;
    }
}

void
execute_command (command_t c, int no_clobber)
{
  switch (c->type)
    {
      case CMD_SEQUENCE: execute_sequence_command (c, no_clobber); break;
      case CMD_AND: execute_and_command (c, no_clobber); break;
      case CMD_OR: execute_or_command (c, no_clobber); break;
      case CMD_PIPE: execute_pipe_command (c, no_clobber); break;
      case CMD_SIMPLE: execute_simple_command (c, no_clobber); break;
      case CMD_SUBSHELL: execute_subshell_command (c, no_clobber); break;
    }
}

