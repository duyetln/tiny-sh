#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

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
  while (c-- > 0 && n != NULL)
    n = n->next;

  return ((n != NULL) ? n->value : NULL);
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
  while (c-- > 0 && strm->curr != NULL)
    strm->curr = strm->curr->next;
  return current_token (strm);
}

token_t
backward_token_stream (token_stream_t strm, int c)
{
  while (c-- > 0 && strm->curr != NULL)
    strm->curr = strm->curr->prev;
  return current_token (strm);
}

token_t
skip_token (token_stream_t strm, enum token_type t)
{
  while (strm->curr != NULL && strm->curr->value->type == t)
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

token_stream_t
create_token_stream (int (*next_char) (void *), void *file)
{
  int c = (*next_char)(file);
  int line_num = 1;

  token_stream_t strm = malloc_token_stream;
  strm->head = NULL;
  strm->tail = NULL;
  strm->curr = NULL;

  token_t tkn = NULL;

  while (c != EOF)
    {
      while (isspace (c) && c != '\n')
        c = (*next_char)(file);

      if (c == '\n')
        line_num++;

      if (c == '#')
        {
          do
            {
              c = (*next_char)(file);
            }
          while (c != '\n');
          move_backwards (c, file, 1);
        }
      else if (iswordchar (c))
        {
          // this should be more than enough, hopefully
          int size = 50;
          int count = 0;
          char *buffer = (char *) malloc ((sizeof (char)) * size);
          while (iswordchar (c))
            {
              if (count >= size)
                {
                  size *= 2;
                  char *newbuffer = (char *) malloc ((sizeof (char)) * size);
                  strncpy (newbuffer, buffer, count);
                  free (buffer);
                  buffer = newbuffer;
                }

              buffer[count++] = c;
              c = (*next_char)(file);
            }

          move_backwards (c, file, 1);

          tkn = malloc_token;
          tkn->type = WORD;
          tkn->value = strndup (buffer, count);

          free (buffer);
        }
      else if (c == '<')
        {
          tkn = malloc_token;
          tkn->type = INPUT;
          tkn->value = strdup ("<");
        }
      else if (c == '>')
        {
          tkn = malloc_token;
          tkn->type = OUTPUT;
          tkn->value = strdup (">");
        }
      else if (c == '(')
        {
          tkn = malloc_token;
          tkn->type = OPENPAREN;
          tkn->value = strdup ("(");
        }
      else if (c == ')')
        {
          tkn = malloc_token;
          tkn->type = CLOSEPAREN;
          tkn->value = strdup (")");
        }
      else if (c == ';')
        {
          tkn = malloc_token;
          tkn->type = SEMICOLON;
          tkn->value = strdup (";");
        }
      else if (c == '\n')
        {
          tkn = malloc_token;
          tkn->type = NEWLINE;
          tkn->value = strdup ("\n");
        }
      else if (c == '|')
        {
          c = (*next_char)(file);
          if (c == '|')
            {
              tkn = malloc_token;
              tkn->type = OR;
              tkn->value = strdup ("||");
            }
          else
            {
              tkn = malloc_token;
              tkn->type = PIPE;
              tkn->value = strdup ("|");

              move_backwards (c, file, 1);
            }
        }
      else if (c == '&')
        {
          c = (*next_char)(file);
          if (c == '&')
            {
              tkn = malloc_token;
              tkn->type = AND;
              tkn->value = strdup ("&&");
            }
          else
            {
              printf("%d: missing &\n", line_num);
              move_backwards (c, file, 1);
            }
        }
      else
        {
          printf("%d: unrecognized char: %c\n", line_num, c);
        }

      if (tkn != NULL)
        {
          add_token (strm, tkn);
          tkn = NULL;
        }

      c = (*next_char)(file);
    }

  return strm;
}
