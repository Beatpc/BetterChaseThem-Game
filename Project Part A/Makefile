CC = gcc
CFLAGS = -Wall -g 

all: chase-client bot-client prize-client server

chase-client: chase_client/chase-client.c
	$(CC) $(CFLAGS) -o chase-client chase_client/chase-client.c -lncurses

bot-client: bots_client/bot-client.c
	$(CC) $(CFLAGS) -o bot-client bots_client/bot-client.c -lncurses

server: main_server/server.c
	$(CC) $(CFLAGS) -o server main_server/server.c -lncurses

prize-client: prize_client/prize-client.c
	$(CC) $(CFLAGS) -o prize-client prize_client/prize-client.c -lncurses

clean:
	rm chase-client bot-client prize-client server
