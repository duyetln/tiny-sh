#ifndef COMMAND_H
#define COMMAND_H

enum io_op
{
  IO_INPUT,      // "<"
  IO_DUPIN,      // "<&"
  IO_IODUAL,     // "<>"
  IO_OUTPUT,     // ">"
  IO_DUPOUT,     // ">&"
  IO_APPEND,     // ">>"
  IO_CLOBBER,    // ">|"
};

struct io_node
{
  enum io_op op;
  char *io_num;
  char *word;
  struct io_node *next;
};

enum command_type
{
  CMD_AND,        // A && B
  CMD_SEQUENCE,   // A ; B
  CMD_OR,         // A || B
  CMD_PIPE,       // A | B
  CMD_SIMPLE,     // a simple command
  CMD_SUBSHELL,   // ( A )
};

struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  struct io_node *io_head;
  struct io_node *io_tail;

  union
  {
    struct command *command[2];
    struct command *subshell_command;
    char **word;
  } u;
};

typedef struct command *command_t;
typedef struct io_node *io_node_t;
#endif
