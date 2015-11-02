#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <error.h>

#include "token_stream.h"

// move backwars unless EOF
#define move_backwards(c, file, count) ((c) != EOF && fseek(file, ftell(file) - (count), SEEK_SET))
#define malloc_token ((token_t) malloc (sizeof (struct token)))
#define malloc_token_node ((token_node_t) malloc (sizeof (struct token_node)))
#define malloc_token_stream ((token_stream_t) malloc (sizeof (struct token_stream)))
#define iswordchar(c) (isalnum(c) || \
  c == '!' || c == '%' || c == '+' || c == ',' || \
  c == '-' || c == '.' || c == '/' || c == ':' || \
  c == '@' || c == '^' || c == '_')

token_t
next_token (token_stream_t strm)
{
  return forward_token_stream (strm, 1);
}

token_t
current_token (token_stream_t strm)
{
  return peek_token (strm, 0);
}

token_t
peek_token (token_stream_t strm, int c)
{
  token_node_t n = strm->curr;
  while (c-- > 0 && n->value->type != ETKN)
    n = n->next;

  return n->value;
}

token_t
reset_token_stream (token_stream_t strm)
{
  strm->curr = strm->head;
  return current_token (strm);
}

token_t
forward_token_stream (token_stream_t strm, int c)
{
  while (c-- > 0 && strm->curr->value->type != ETKN)
    strm->curr = strm->curr->next;
  return current_token (strm);
}

token_t
backward_token_stream (token_stream_t strm, int c)
{
  while (c-- > 0 && strm->curr != strm->head)
    strm->curr = strm->curr->prev;
  return current_token (strm);
}

token_t
skip_token (token_stream_t strm, enum token_type t)
{
  while (strm->curr->value->type != ETKN && strm->curr->value->type == t)
    strm->curr = strm->curr->next;
  return current_token (strm);
}

void
destroy_token_stream (token_stream_t strm)
{
  if (strm != NULL)
    {
      if (strm->head != NULL && strm->tail != NULL)
        {
          while (strm->tail != strm->head)
            {
              free (strm->tail->value->value);
              free (strm->tail->value);
              strm->tail = strm->tail->prev;
              free (strm->tail->next);
            }

          free (strm->tail->value->value);
          free (strm->tail->value);
          free (strm->tail);
        }

      free (strm);
    }
}

token_node_t
add_token (token_stream_t strm, token_t tkn)
{
  token_node_t node = malloc_token_node;
  node->value = tkn;
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

token_t
create_token (enum token_type type, char *value, int line)
{
  token_t tkn = malloc_token;
  tkn->type = type;
  tkn->value = value;
  tkn->line = line;

  return tkn;
}

void
bufferadd (char **buffer, int *size, int *count, char c)
{
  if (*buffer == NULL)
    {
      *buffer = (char *) malloc (*size * sizeof (char));
      *count = 0;
    }

  if (*count >= *size)
    {
      *size *= 2;
      char *newbuffer = (char *) malloc (*size * sizeof (char));
      strncpy (newbuffer, *buffer, *count);
      free (*buffer);
      *buffer = newbuffer;
    }

  (*buffer)[(*count)++] = c;
}

token_stream_t
create_token_stream (int (*next_char) (void *), void *file)
{
  int c = (*next_char) (file);
  int line = 1;

  token_stream_t strm = malloc_token_stream;
  strm->head = NULL;
  strm->tail = NULL;
  strm->curr = NULL;

  token_t tkn = NULL;

  while (c != EOF)
    {
      while (isspace (c))
        {
          if (c == '\n')
            line++;
          c = (*next_char) (file);
        }

      if (c == '#')
        {
          do
            {
              c = (*next_char) (file);
            }
          while (c != '\n');
          move_backwards (c, file, 1);
        }
      else if (iswordchar (c))
        {
          // this should be more than enough, hopefully
          int size = 50;
          int count = 0;
          char *buffer = NULL;
          while (iswordchar (c))
            {
              bufferadd (&buffer, &size, &count, c);
              c = (*next_char) (file);
            }

          move_backwards (c, file, 1);
          tkn = create_token (WORD, strndup (buffer, count), line);
          free (buffer);
        }
      else if (c == '<')
        {
          c = (*next_char) (file);
          if (c == '>')
            tkn = create_token (IODUAL, strdup ("<>"), line);
          else
            {
              tkn = create_token (INPUT, strdup ("<"), line);
              move_backwards (c, file, 1);
            }
        }
      else if (c == '>')
        {
          c = (*next_char) (file);
          if (c == '>')
            tkn = create_token (APPEND, strdup (">>"), line);
          else if (c == '|')
            tkn = create_token (CLOBBER, strdup (">|"), line);
          else
            {
              tkn = create_token (OUTPUT, strdup (">"), line);
              move_backwards (c, file, 1);
            }
        }
      else if (c == '(')
        tkn = create_token (OPENPAREN, strdup ("("), line);
      else if (c == ')')
        tkn = create_token (CLOSEPAREN, strdup (")"), line);
      else if (c == ';')
        tkn = create_token (SEMICOLON, strdup (";"), line);
      else if (c == '|')
        {
          c = (*next_char) (file);
          if (c == '|')
            tkn = create_token (OR, strdup ("||"), line);
          else
            {
              tkn = create_token (PIPE, strdup ("|"), line);
              move_backwards (c, file, 1);
            }
        }
      else if (c == '&')
        {
          c = (*next_char) (file);
          if (c == '&')
            tkn = create_token (AND, strdup ("&&"), line);
          else if (isdigit (c))
            {
              int size = 3;
              int count = 0;
              char *buffer = NULL;
              while (isdigit (c))
                {
                  bufferadd (&buffer, &size, &count, c);
                  c = (*next_char) (file);
                }

              move_backwards (c, file, 1);
              tkn = create_token (IONUMBER, strndup (buffer, count), line);
              free (buffer);
            }
          else
            {
              error (1, 0, "%d: unrecognized token &\n", line);
              move_backwards (c, file, 1);
            }
        }
      else
        {
          error (1, 0, "%d: unrecognized token %c\n", line, c);
        }

      if (tkn != NULL)
        {
          add_token (strm, tkn);
          tkn = NULL;
        }

      c = (*next_char) (file);
    }

  tkn = create_token (ETKN, strdup ("EOF"), line);

  add_token (strm, tkn);
  tkn = NULL;

  return strm;
}
