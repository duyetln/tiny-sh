// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "parser.h"

command_stream_t
make_command_stream (int (*get_next_byte) (void *), void *get_next_byte_argument)
{
  return create_command_stream (get_next_byte, get_next_byte_argument);
}

command_t
read_command_stream (command_stream_t s)
{
  return next_command (s);
}
