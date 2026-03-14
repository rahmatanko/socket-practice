CC=gcc
CFLAGS=-Wall -Wextra -Werror -fsanitize=address -g

all: server client

server: server.c
	$(CC) $(CFLAGS) -o $@ $<

client: client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f server client

debug: server.c
	$(CC) $(CFLAGS) -ggdb -o server $<

run: server
	./server

.PHONY: all clean debug run