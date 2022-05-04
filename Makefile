CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

SERVER_OBJECTS = server.o
CLIENT_OBJECTS = client.o

.PHONY: all clean
all: server client

server: $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

client: $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


server.o: server.c
client.o: client.c


clean:
	rm -rf *.o all server client
