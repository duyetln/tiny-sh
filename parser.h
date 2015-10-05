command_stream_t
create_command_stream (int (*next_char) (void *), void *file);

void
destroy_command_stream (command_stream_t strm);

command_t
next_command (command_stream_t strm);
