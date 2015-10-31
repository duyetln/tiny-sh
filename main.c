// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command_utility.h"
#include "command_stream.h"
#include "concurrent_commands.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
  options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);

  command_stream_t command_stream = create_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;
  int status;

  if (time_travel)
    status = parallelize_command_stream (command_stream);
  else
    {
      while ((command = current_command (command_stream)))
        {
          if (print_tree)
            {
              printf ("# %d\n", command_number++);
              print_command (command);
            }
          else
            {
              last_command = command;
              execute_command (command);
            }

          next_command (command_stream);
        }

      status = print_tree || !last_command ? 0 : command_status (last_command);
    }

  fclose (script_stream);
  destroy_command_stream (command_stream);

  return status;
}
