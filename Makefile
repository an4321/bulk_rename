CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -D_DEFAULT_SOURCE

all: rn

rn: rn.c
	$(CC) $(CFLAGS) rn.c -o rn

clean:
	rm rn

install: rn
	mkdir -p ~/.local/bin
	cp -f rn ~/.local/bin

uninstall:
	rm -f ~/.local/bin/rn
