CC = gcc

CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra

all: server

server:
	$(CC) $(CFLAGS) server.c -o server

clean:
	rm server