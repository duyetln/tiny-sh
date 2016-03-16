CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused -Werror

all: tinysh

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

TINYSH_SOURCES = \
  main.c \
  token_stream.c \
  command_stream.c \
  command_utility.c \
  concurrent_commands.c
TINYSH_OBJECTS = $(subst .c,.o,$(TINYSH_SOURCES))


tinysh: $(TINYSH_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(TINYSH_OBJECTS)

execute-command.o main.o print-command.o read-command.o: command.h command_utility.h token_stream.h command_stream.h concurrent_commands.h
execute-command.o print-command.o read-command.o: command.h command_utility.h token_stream.h command_stream.h concurrent_commands.h

check: $(TEST_BASES)

$(TEST_BASES): tinysh
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp tinysh $(DISTDIR)

.PHONY: all check $(TEST_BASES) clean
