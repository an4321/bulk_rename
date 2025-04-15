CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -D_DEFAULT_SOURCE
SRCS = rn.c db.c util.c listify.c

all: rn

rn: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o rn

test: $(SRCS) 
	$(CC) $(CFLAGS) -g $(SRCS) -o rn
	valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./rn

clean:
	rm rn

install: rn
	mkdir -p ~/.local/bin
	cp -f rn ~/.local/bin

uninstall:
	rm -f ~/.local/bin/rn
