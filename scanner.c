#include <stdio.h>
#include "scanner.h"

#define malloc_token ((token_t) malloc (sizeof (struct token))
#define malloc_token_node ((token_node_t) malloc (sizeof (struct token_node)))
#define malloc_token_stream ((token_stream_t) malloc (sizeof (struct token_stream)))
#define iswordchar(c) (isalnum(c) || \
  c == '!' || c == '%' || c == '+' || c == ',' || \
  c == '-' || c == '.' || c == '/' || c == ':' || \
  c == '@' || c == '^' || c == '_')

token_stream_t
make_token_stream (int (*next_char) (void *), void *file)
{
  char c = (*next_char)(file);

  token_stream_t strm = malloc_token_stream;
  strm->head = NULL;
  strm->tail = NULL;
  strm->curr = NULL;

  token_t tkn = NULL;

  while (c != EOF)
    {
      while (isspace(c) && c != '\n')
        c = (*next_char)(file);

      if (iswordchar(c))
        {
          // this should be more than enough, hopefully
          int size = 50;
          int index = 0;
          char *buffer = (char *) malloc ((sizeof (char)) * size);
          while (iswordchar(c))
            {
              if (index >= size)
                {
                  size *= 2;
                  char *newbuffer = (char *) malloc ((sizeof (char)) * size);
                  strncpy(newbuffer, buffer, index);
                  free(buffer);
                  buffer = newbuffer;
                }

              buffer[index++] = c;
              c = (*next_char)(file);
            }

          // move backwards 1 char
          fseek(ftell(file) - 1);

          tkn = malloc_token;
          tkn->type = WORD;
          tkn->value = strndup(buffer, index);

          free(buffer);
        }
      else if (c == '<')
        {
          tkn = malloc_token;
          tkn->type = INPUT;
          tkn->value = strdup("<");
        }
      else if (c == '>')
        {
          tkn = malloc_token;
          tkn->type = OUTPUT;
          tkn->value = strdup(">");
        }
      else if (c == '(')
        {
          tkn = malloc_token;
          tkn->type = OPENPAREN;
          tkn->value = strdup("(");
        }
      else if (c == ')')
        {
          tkn = malloc_token;
          tkn->type = CLOSEPAREN;
          tkn->value = strdup(")");
        }
      else if (c == ';')
        {
          tkn = malloc_token;
          tkn->type = SEMICOLON;
          tkn->value = strdup(";");
        }
      else if (c == '\n')
        {
          tkn = malloc_token;
          tkn->type = NEWLINE;
          tkn->value = strdup("\n");
        }
      else if (c == '|')
        {
          c = (*next_char)(file);
          if (c == '|')
            {
              tkn = malloc_token;
              tkn->type = OR;
              tkn->value = strdup("||");
            }
          else
            {
              tkn = malloc_token;
              tkn->type = PIPE;
              tkn->value = strdup("|");

              // move backwards 1 char
              fseek(ftell(file) - 1);
            }
        }
      else if (c == '&')
        {
          c = (*next_char)(file);
          if (c == '&')
            {
              tkn = malloc_token;
              tkn->type = AND;
              tkn->value = strdup("&&");
            }
          else
            {
              printf("Unrecognized char: &");

              // move backwards 1 char
              fseek(ftell(file) - 1);
            }
        }
      else
        {
          printf("Unrecognized char: %c", c);
        }

      add_token(strm, tkn);
      tkn = NULL;
      c = (*next_char)(file);
    }

  return strm;
}

void
destroy_token_stream(token_stream_t strm)
{
  while(strm->tail != NULL)
    {
      free(strm->tail->value->value);
      free(strm->tail->value);
      strm->tail = strm->tail->prev;
      free(strm->tail->next);
    }

  free(strm);
}

token_node_t
add_token (token_stream_t strm, token_t tkn)
{
  token_node_t node = malloc_token_node;
  node->value = tkn;
  node->prev = NULL;
  node->next = NULL;

  if (strm->head == NULL || strm->tail == NULL || strm->curr == NULL)
    {
      strm->head = node;
      strm->tail = node;
      strm->curr = node;
    }

  strm->curr->next = node;
  node->prev = strm->curr;
  strm->curr = node;
  strm->tail = node;

  return node;
}


