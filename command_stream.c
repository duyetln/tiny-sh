#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <error.h>

#include "token_stream.h"
#include "command_stream.h"

#define malloc_command ((command_t) malloc (sizeof (struct command)))
#define malloc_command_node ((command_node_t) malloc (sizeof (struct command_node)))
#define malloc_command_stream ((command_stream_t) malloc (sizeof (struct command_stream)))
#define istkn(tkn, tp) ((tkn) != NULL && (tkn)->type == tp)
#define isiotkn(t) (istkn (t, TKN_INPUT) || istkn (t, TKN_DUPIN) || istkn (t, TKN_IODUAL) || istkn (t, TKN_OUTPUT) || istkn (t, TKN_DUPOUT) || istkn (t, TKN_APPEND) || istkn (t, TKN_CLOBBER))

command_t
parse_command_sequence (token_stream_t strm);

command_t
create_command (enum command_type t)
{
  command_t cmd = malloc_command;
  cmd->type = t;
  cmd->status = -1;
  cmd->io_head = NULL;
  cmd->io_tail = NULL;

  if (t == CMD_SIMPLE)
    cmd->u.word = NULL;
  else if (t == CMD_SUBSHELL)
    cmd->u.subshell_command = NULL;
  else // the remaining command types
    {
      cmd->u.command[0] = NULL;
      cmd->u.command[1] = NULL;
    }

  return cmd;
}

void
assert_token (token_stream_t strm, enum token_type t)
{
  if (!istkn (current_token (strm), t))
    {
      char *tw;
      switch (t)
        {
          case TKN_WORD: tw = "word"; break;
          case TKN_IONUMBER: tw = "io number"; break;
          case TKN_INPUT: tw = "'<'"; break;
          case TKN_DUPIN: tw = "'<&'"; break;
          case TKN_IODUAL: tw = "'<>'"; break;
          case TKN_OUTPUT: tw = "'>'"; break;
          case TKN_DUPOUT: tw = "'>&'"; break;
          case TKN_APPEND: tw = "'>>'"; break;
          case TKN_CLOBBER: tw = "'>|'"; break;
          case TKN_PIPE: tw = "'|'"; break;
          case TKN_OPENPAREN: tw = "'('"; break;
          case TKN_CLOSEPAREN: tw = "')'"; break;
          case TKN_AND: tw = "'&&'"; break;
          case TKN_OR: tw = "'||'"; break;
          case TKN_SEMICOLON: tw = "';'"; break;
          case TKN_DBLSEMICLN: tw = "';;'"; break;
          case TKN_EOF: tw = "eof"; break;
        }

      error (1, 0, "%d: expecting %s token\n", current_token (strm)->line, tw);
    }
}

command_t
parse_simple_command (token_stream_t strm)
{
  // printf("parse simple command: %s\n", current_token (strm)->value);
  int c = 0;
  int i = 0;
  while (istkn (current_token (strm), TKN_WORD))
    {
      c++;
      next_token (strm);
    }

  command_t cmd = create_command (CMD_SIMPLE);
  cmd->u.word = (char**) malloc ((c + 1) * sizeof (char *));

  backward_token_stream (strm, c);
  while (istkn (current_token (strm), TKN_WORD))
    {
      cmd->u.word[i] = strdup (current_token (strm)->value);
      i++;
      next_token (strm);
    }
  cmd->u.word[i] = NULL;

  return cmd;
}

command_t
parse_io_redirection (token_stream_t strm, command_t cmd)
{
  // printf("parse io redirection: %s\n", current_token (strm)->value);
  io_node_t io;
  io = (io_node_t) malloc (sizeof (struct io_node));
  io->io_num = NULL;
  io->word = NULL;
  io->next = NULL;

  token_t c = current_token (strm);
  token_t n = next_token (strm);

  if (istkn (c, TKN_IONUMBER))
    {
      io->io_num = strdup (c->value);
      c = current_token (strm);
      n = next_token (strm);
    }

  if (istkn (c, TKN_INPUT))
    {
      assert_token (strm, TKN_WORD);
      io->op = IO_INPUT;
    }
  else if (istkn (c, TKN_DUPIN))
    {
      assert_token (strm, TKN_IONUMBER);
      io->op = IO_DUPIN;
    }
  else if (istkn (c, TKN_IODUAL))
    {
      assert_token (strm, TKN_WORD);
      io->op = IO_IODUAL;
    }
  else if (istkn (c, TKN_OUTPUT))
    {
      assert_token (strm, TKN_WORD);
      io->op = IO_OUTPUT;
    }
  else if (istkn (c, TKN_DUPOUT))
    {
      assert_token (strm, TKN_IONUMBER);
      io->op = IO_DUPOUT;
    }
  else if (istkn (c, TKN_APPEND))
    {
      assert_token (strm, TKN_WORD);
      io->op = IO_APPEND;
    }
  else if (istkn (c, TKN_CLOBBER))
    {
      assert_token (strm, TKN_WORD);
      io->op = IO_CLOBBER;
    }
  else if (io->io_num != NULL)
    error (1, 0, "%d: expecting io token\n", c->line);

  io->word = strdup (n->value);

  if (cmd->io_head == NULL || cmd->io_tail == NULL)
    {
      cmd->io_head = io;
      cmd->io_tail = io;
    }
  else
    {
      cmd->io_tail->next = io;
      cmd->io_tail = io;
    }

  next_token (strm);

  return cmd;
}

command_t
parse_subshell_command (token_stream_t strm)
{
  // printf("parse subshell command: %s\n", current_token (strm)->value);
  assert_token (strm, TKN_OPENPAREN);
  next_token (strm);

  command_t cmd = create_command (CMD_SUBSHELL);
  cmd->u.subshell_command = parse_command_sequence (strm);

  assert_token (strm, TKN_CLOSEPAREN);
  next_token (strm);

  return cmd;
}

command_t
parse_command (token_stream_t strm)
{
  // printf("parse command: %s\n", current_token (strm)->value);
  command_t cmd;
  token_t t = current_token (strm);

  if (istkn (t, TKN_WORD))
    cmd = parse_simple_command (strm);
  else if (istkn (t, TKN_OPENPAREN))
    cmd = parse_subshell_command (strm);
  else
    error (1, 0, "%d: expecting word or '(' token\n", t->line);

  t = current_token (strm);
  while (istkn (t, TKN_IONUMBER) || isiotkn (t))
    {
      cmd = parse_io_redirection (strm, cmd);
      t = current_token (strm);
    }

  return cmd;
}

command_t
parse_pipelines (token_stream_t strm)
{
  // printf("parse pipelines: %s\n", current_token (strm)->value);
  token_t t;
  command_t lft;
  command_t rgt;
  command_t pipe;

  lft = parse_command (strm);
  t = current_token (strm);

  while (istkn (t, TKN_PIPE))
    {
      next_token (strm);

      pipe = create_command (CMD_PIPE);
      rgt = parse_command (strm);
      pipe->u.command[0] = lft;
      pipe->u.command[1] = rgt;
      lft = pipe;

      t = current_token (strm);
    }

  return lft;
}

command_t
parse_logicals (token_stream_t strm)
{
  // printf("parse logicals: %s\n", current_token (strm)->value);
  token_t t;
  command_t lft;
  command_t rgt;
  command_t lgcl;

  lft = parse_pipelines (strm);
  t = current_token (strm);

  while (istkn (t, TKN_AND) || istkn (t, TKN_OR))
    {
      next_token (strm);

      if (istkn (t, TKN_AND))
        lgcl = create_command (CMD_AND);
      else if (istkn (t, TKN_OR))
        lgcl = create_command (CMD_OR);

      rgt = parse_pipelines (strm);
      lgcl->u.command[0] = lft;
      lgcl->u.command[1] = rgt;
      lft = lgcl;

      t = current_token (strm);
    }

  return lft;
}

command_t
parse_command_sequence (token_stream_t strm)
{
  // printf("parse command sequence: %s\n", current_token (strm)->value);
  command_t lft;
  command_t rgt;
  command_t seq;

  lft = parse_logicals (strm);

  while (istkn (current_token (strm), TKN_SEMICOLON))
    {
      next_token (strm);
      rgt = parse_logicals (strm);
      seq = create_command (CMD_SEQUENCE);
      seq->u.command[0] = lft;
      seq->u.command[1] = rgt;
      lft = seq;
    }

  return lft;
}

command_node_t
add_command (command_stream_t strm, command_t cmd)
{
  command_node_t node = malloc_command_node;
  node->value = cmd;
  node->prev = NULL;
  node->next = NULL;

  if (strm->head == NULL || strm->tail == NULL)
    {
      strm->head = node;
      strm->tail = node;
      strm->curr = node;
    }
  else
    {
      strm->tail->next = node;
      node->prev = strm->tail;
      strm->tail = node;
    }

  return node;
}

command_stream_t
parse (token_stream_t strm)
{
  // printf("parse: %s\n", current_token (strm)->value);
  command_stream_t cmd_strm = malloc_command_stream;
  cmd_strm->head = NULL;
  cmd_strm->tail = NULL;
  cmd_strm->curr = NULL;

  while (!istkn (current_token (strm), TKN_EOF))
    {
      add_command (cmd_strm, parse_command_sequence (strm));

      while (istkn (current_token (strm), TKN_DBLSEMICLN))
        {
          next_token (strm);
          add_command (cmd_strm, parse_command_sequence (strm));
        }
    }

  return cmd_strm;
}

command_stream_t
create_command_stream (int (*next_char) (void *), void *file)
{
  token_stream_t tkn_strm = create_token_stream (next_char, file);
  command_stream_t cmd_strm = parse (tkn_strm);
  destroy_token_stream (tkn_strm);
  return cmd_strm;
}

void
destroy_command (command_t cmd)
{
  switch (cmd->type)
    {
      case CMD_SEQUENCE:
      case CMD_AND:
      case CMD_OR:
      case CMD_PIPE:
        {
          destroy_command (cmd->u.command[0]);
          destroy_command (cmd->u.command[1]);
          break;
        }

      case CMD_SIMPLE:
        {
          char **w = cmd->u.word;
          while (*w != NULL)
            {
              free (*w);
              w++;
            }
          free (cmd->u.word);
          break;
        }

      case CMD_SUBSHELL:
        {
          destroy_command (cmd->u.subshell_command);
          break;
        }
    }

  io_node_t c;
  io_node_t n;

  c = cmd->io_head;
  while (c != NULL)
    {
      if (c->io_num)
        free (c->io_num);
      free (c->word);

      n = c->next;
      free (c);
      c = n;
    }

  free (cmd);
}

void
destroy_command_stream (command_stream_t strm)
{
  if (strm != NULL)
    {
      if (strm->head != NULL && strm->tail != NULL)
        {
          while (strm->tail != strm->head)
            {
              destroy_command (strm->tail->value);
              strm->tail = strm->tail->prev;
              free (strm->tail->next);
            }

          destroy_command (strm->tail->value);
          free (strm->tail);
        }

      free (strm);
    }
}

command_t
next_command (command_stream_t strm)
{
  if (strm->curr != NULL)
    {
      strm->curr = strm->curr->next;
    }
  return current_command (strm);
}

command_t
current_command (command_stream_t strm)
{
  return strm->curr != NULL ? strm->curr->value : NULL;
}

command_t
reset_command_stream (command_stream_t strm)
{
  strm->curr = strm->head;
  return current_command (strm);
}
