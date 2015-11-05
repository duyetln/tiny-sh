#ifndef COMMAND_UTILITY_H
#define COMMAND_UTILITY_H

#include "command.h"

/* Print a command to stdout, for debugging.  */
void
print_command (command_t cmd);

void
execute_command (command_t cmd, int no_clobber);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int
command_status (command_t cmd);

#endif
