#include "command.h"

/* Print a command to stdout, for debugging.  */
void
print_command (command_t cmd);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
void
execute_command (command_t cmd, int f);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int
command_status (command_t cmd);
