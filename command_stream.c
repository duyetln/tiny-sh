#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <error.h>

#include "token_stream.h"
#include "command_stream.h"

#define malloc_command ((command_t) malloc (sizeof (struct command)))
#define malloc_command_node ((command_node_t) malloc (sizeof (struct command_node)))
#define malloc_command_stream ((command_stream_t) malloc (sizeof (struct command_stream)))

#define not_null(ptr) ((ptr) != NULL)
// used in parse_command_sequence to detect seperator patterns
#define sep_case1(curr, next1) (not_null(curr) && \
  not_null(next1) && \
  (curr)->type == TKN_NEWLINE && \
  (next1)->type != TKN_NEWLINE)
#define sep_case2(curr, next1) (not_null(curr) && \
  not_null(next1) && \
  (curr)->type == TKN_SEMICOLON && \
  (next1)->type != TKN_NEWLINE)
#define sep_case3(curr, next1, next2) (not_null(curr) && \
  not_null(next1) && \
  not_null(next2) && \
  (curr)->type == TKN_SEMICOLON && \
  (next1)->type == TKN_NEWLINE && \
  (next2)->type != TKN_NEWLINE)

command_t
parse_command_sequence (token_stream_t strm);

command_t
create_command (enum command_type t)
{
  command_t cmd = malloc_command;
  cmd->type = t;
  cmd->status = -1;
  cmd->input = NULL;
  cmd->output = NULL;

  if (t == SIMPLE_COMMAND)
    cmd->u.word = NULL;
  else if (t == SUBSHELL_COMMAND)
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
  if (current_token (strm)->type != t)
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
  while (current_token (strm)->type == TKN_WORD)
    {
      c++;
      next_token (strm);
    }

  command_t cmd = create_command (SIMPLE_COMMAND);
  cmd->u.word = (char**) malloc ((c + 1) * sizeof (char *));

  backward_token_stream (strm, c);
  while (current_token (strm)->type == TKN_WORD)
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
  token_t c = current_token (strm);
  token_t n = next_token (strm);
  char **w = NULL;

  assert_token (strm, TKN_WORD);

  if (c->type == TKN_INPUT)
    w = &(cmd->input);
  else if (c->type == TKN_OUTPUT)
    w = &(cmd->output);

  // handle the case where there are more than <'s or >'s
  if (*w != NULL)
    free (*w);
  *w = strdup (n->value);

  next_token (strm);

  return cmd;
}

command_t
parse_subshell_command (token_stream_t strm)
{
  // printf("parse subshell command: %s\n", current_token (strm)->value);
  assert_token (strm, TKN_OPENPAREN);
  next_token (strm);
  skip_token (strm, TKN_NEWLINE);

  command_t cmd = create_command (SUBSHELL_COMMAND);
  cmd->u.subshell_command = parse_command_sequence (strm);

  skip_token (strm, TKN_NEWLINE);
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

  if (t->type == TKN_WORD)
    cmd = parse_simple_command (strm);
  else if (t->type == TKN_OPENPAREN)
    cmd = parse_subshell_command (strm);
  else
    error (1, 0, "%d: expecting word or '(' token\n", t->line);

  t = current_token (strm);
  while (t->type == TKN_INPUT || t->type == TKN_OUTPUT)
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

  while (t->type == TKN_PIPE)
    {
      next_token (strm);
      skip_token (strm, TKN_NEWLINE);

      pipe = create_command (PIPE_COMMAND);
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

  while (t->type == TKN_AND || t->type == TKN_OR)
    {
      next_token (strm);
      skip_token (strm, TKN_NEWLINE);

      if (t->type == TKN_AND)
        lgcl = create_command (AND_COMMAND);
      else if (t->type == TKN_OR)
        lgcl = create_command (OR_COMMAND);

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

  token_t curr = current_token (strm);
  token_t next1 = peek_token (strm, 1);
  token_t next2 = peek_token (strm, 2);

  while (sep_case1 (curr, next1) ||
    sep_case2 (curr, next1) ||
    sep_case3 (curr, next1, next2))
    {
      if (sep_case1 (curr, next1) || sep_case2 (curr, next1))
        forward_token_stream (strm, 1);
      else if (sep_case3 (curr, next1, next2))
        forward_token_stream (strm, 2);

      if (current_token (strm)->type != TKN_EOF)
        {
          seq = create_command (SEQUENCE_COMMAND);
          rgt = parse_logicals (strm);
          seq->u.command[0] = lft;
          seq->u.command[1] = rgt;
          lft = seq;
        }

      curr = current_token (strm);
      next1 = peek_token (strm, 1);
      next2 = peek_token (strm, 2);
    }

  if (not_null (curr) && curr->type == TKN_SEMICOLON)
    next_token (strm);

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

  while (current_token (strm)->type != TKN_EOF)
    {
      skip_token (strm, TKN_NEWLINE);

      if (current_token (strm)->type != TKN_EOF)
        {
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
      case SEQUENCE_COMMAND:
      case AND_COMMAND:
      case OR_COMMAND:
      case PIPE_COMMAND:
        {
          destroy_command (cmd->u.command[0]);
          destroy_command (cmd->u.command[1]);
          break;
        }

      case SIMPLE_COMMAND:
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

      case SUBSHELL_COMMAND:
        {
          destroy_command (cmd->u.subshell_command);
          break;
        }
    }

  if (cmd->input != NULL)
    free (cmd->input);
  if (cmd->output != NULL)
    free (cmd->output);

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
