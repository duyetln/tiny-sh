#include "command-test.h"

command_stream_t
create_command_stream (int (*next_char) (void *), void *file);

void
destroy_command_stream (command_stream_t strm);
