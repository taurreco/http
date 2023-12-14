CC = gcc

CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra

all: server check_request

server:
	$(CC) $(CFLAGS) server.c http.c -o server

check_request:
	$(CC) $(CFLAGS) -Iunity -I. tests/check_request.c unity/unity.c -o tests/check_request

clean:
	rm server
	rm tests/check_request