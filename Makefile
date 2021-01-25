CC=gcc
CFLAGS=-Wall -Werror -pedantic -std=c11 -g -ggdb

all: mymake makefile_parser_driver

# Main executable
mymake: mymake_main.o mymake.o digraph.o makefile_parser.o util.o
	$(CC) $(CFLAGS) -o mymake mymake_main.o mymake.o digraph.o makefile_parser.o util.o

# Main file
mymake_main.o: mymake_main.c mymake.h makefile_parser.h
	$(CC) $(CFLAGS) -c mymake_main.c

# Mymake file
mymake.o: mymake.c mymake.h digraph.h util.h
	$(CC) $(CFLAGS) -c mymake.c

# Digraph file
digraph.o: digraph.c digraph.h
	$(CC) $(CFLAGS) -c digraph.c

# Makefile Driver Executable
makefile_parser_driver: makefile_parser_driver.o makefile_parser.o
	$(CC) $(CFLAGS) -o makefile_parser_driver makefile_parser_driver.o makefile_parser.o

# Makefile Driver file
makefile_parser_driver.o: makefile_parser_driver.c makefile_parser.h
	$(CC) $(CFLAGS) -c makefile_parser_driver.c

# Makefile parser file
makefile_parser.o: makefile_parser.c makefile_parser.h
	$(CC) $(CFLAGS) -c makefile_parser.c

# Utility file
util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

.PHONY: clean
clean:
	-rm -f *.o
	-rm -f mymake
	-rm -f makefile_parser_driver


