#include "command.h"

struct command_node
{
  struct command *value;
  struct command_node *prev;
  struct command_node *next;
};

struct command_stream
{
  struct command_node *head;
  struct command_node *tail;
  struct command_node *curr;
};

typedef struct command_node *command_node_t;
typedef struct command_stream *command_stream_t;

command_stream_t
create_command_stream (int (*next_char) (void *), void *file);

void
destroy_command_stream (command_stream_t strm);

command_t
next_command (command_stream_t strm);

command_t
current_command (command_stream_t strm);

command_t
reset_command_stream (command_stream_t strm);
