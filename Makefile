# CS 111 Lab 1 Makefile

CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused -Werror
LAB = 1
DISTDIR = lab1-$(USER)

all: timetrash

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

TIMETRASH_SOURCES = \
  alloc.c \
  main.c \
  token_stream.c \
  command_stream.c \
  command_utility.c
TIMETRASH_OBJECTS = $(subst .c,.o,$(TIMETRASH_SOURCES))

DIST_SOURCES = \
  $(TIMETRASH_SOURCES) alloc.h command.h command_utility.h token_stream.h command_stream.h Makefile \
  $(TESTS) check-dist README

timetrash: $(TIMETRASH_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(TIMETRASH_OBJECTS)

alloc.o: alloc.h
execute-command.o main.o print-command.o read-command.o: command.h command_utility.h token_stream.h command_stream.h
execute-command.o print-command.o read-command.o: command.h command_utility.h token_stream.h command_stream.h

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) check-dist
	rm -fr $(DISTDIR)
	tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	./check-dist $(DISTDIR)
	mv $@.tmp $@

check: $(TEST_BASES)

$(TEST_BASES): timetrash
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp timetrash $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean
