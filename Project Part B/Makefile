CC = gcc
CFLAGS = -Wall -g 

all: chase-client server

chase-client: chase_client/chase-client.c
	$(CC) $(CFLAGS) -o chase-client chase_client/chase-client.c -lncurses -lpthread

server: main_server/server.c
	$(CC) $(CFLAGS) -o server main_server/server.c -lncurses -lpthread

clean:
	rm chase-client server