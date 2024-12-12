#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "../variables.h"

int main(int argc, char *argv[]) // arguments: server address
{
    // Socket creation to communicate with server
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket: ");
        exit(-1);
    }
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    sprintf(local_client_addr.sun_path, "%s_%d", argv[1], getpid());
    unlink(local_client_addr.sun_path);
    int err = bind(sock_fd,(const struct sockaddr *)&local_client_addr, sizeof(local_client_addr));
    if (err == -1)
    {
        perror("bind");
        exit(-1);
    }
    // give the caracteristics of the server
    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, argv[1]);
    struct sockaddr_un check_server_addr;
    socklen_t check_server_addr_size = sizeof(struct sockaddr_un);

    // send 'connect' message to server (to prevent the game from having more than 1 prize client)
    message_client message_to_server;
    message_to_server.msg_type = 'c';    // message type: 'connect'
    message_to_server.player_type = 'p'; // player type: 'prize'

    sendto(sock_fd, &message_to_server, sizeof(message_client), 0,
           (const struct sockaddr *)&server_addr, sizeof(server_addr));

    message_server message_from_server;

    while (1) // the prize client is waiting to receive a message from the server 
    {
        recvfrom(sock_fd, &message_from_server, sizeof(message_server), 0, (struct sockaddr *)&check_server_addr, &check_server_addr_size);
        
        if (strcmp(check_server_addr.sun_path, server_addr.sun_path) == 0) // if the message is from the server
        {
            if (message_from_server.msg_type == 'd') /// received a 'disconnect' message from the server
            {   // there must be only one prize client in the game
                printf("Error: There is already a prize client in the game\n");
                close(sock_fd);
                exit(0);
            }else if (message_from_server.msg_type == 'c'){ // received a 'connect' message from the server
                // sends a "movement" to the server every 5 seconds so it knows the prizes must be updated 
                message_to_server.msg_type = 'p'; 
                while (1)
                {
                    sleep(5); // 5 seconds timer
                    sendto(sock_fd, &message_to_server, sizeof(message_client), 0,
                        (const struct sockaddr *)&server_addr, sizeof(server_addr));
                }
            }
        }
    }

}