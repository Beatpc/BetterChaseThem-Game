#include <stdlib.h>
#include <ncurses.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "../variables.h"

WINDOW *message_win;

void draw_player(WINDOW *win, player *player, int delete)
{
    int ch;
    if (delete)
    {
        ch = player->ch;
    }
    else
    {
        ch = ' ';
    }
    int p_x = player->x;
    int p_y = player->y;
    wmove(win, p_y, p_x);
    waddch(win, ch);
}

void draw_health(WINDOW *win, player *player, int delete, int line)
{
    if (player->type == 'b' || player->type == 'p')
    {
        return;
    }
    if (!delete)
    {
        mvwprintw(win, line + 1, 1, "         ");
        mvwprintw(win, (line + 1) - 5, 10, "         ");
        return;
    }
    char str[100] = {'\0'};
    sprintf(str, "%c %d", player->ch, player->health);
    if (line <= 4)
    {
        mvwprintw(win, line + 1, 1, "%s", str);
    }
    else
    {
        mvwprintw(win, (line + 1) - 5, 10, "%s", str);
    }
}

player p1;

int main(int argc, char *argv[])
{
    int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket: ");
        exit(-1);
    }
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    sprintf(local_client_addr.sun_path, "%s_%d", argv[1], getpid());
    unlink(local_client_addr.sun_path);
    int err = bind(sock_fd, (const struct sockaddr *)&local_client_addr, sizeof(local_client_addr));
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
    check_server_addr.sun_family = AF_UNIX;
    strcpy(check_server_addr.sun_path, argv[1]);

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    WINDOW *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);

    int number_of_players = 0;
    int n_bytes;
    player players_data[100];

    message_client message_from_client;
    message_server message_from_server;

    // send 'connect' message to server
    message_from_client.msg_type = 'c';    // message type: 'connect'
    message_from_client.player_type = 'h'; // player type: 'human' (ball)
    sendto(sock_fd, &message_from_client, sizeof(message_client), 0,
           (const struct sockaddr *)&server_addr, sizeof(server_addr));

    // wait for 'ball_information' message
    while (1)
    {
        n_bytes = recvfrom(sock_fd, &message_from_server, sizeof(message_server), 0, (struct sockaddr *)&check_server_addr, &check_server_addr_size);
        // Check if the address belongs to the server, and not another client trying to cheat
        if (strcmp(check_server_addr.sun_path, server_addr.sun_path) == 0)
        {
            if (message_from_server.msg_type == 'b') // check if it is a 'ball_information' message
            {
                // save the field status as only this player, and save the number of players to delete (at first is only one)
                p1.x = message_from_server.x;
                p1.y = message_from_server.y;
                p1.ch = message_from_server.ch;
                p1.health = message_from_server.health;

                number_of_players = 1;
                players_data[0] = p1;

                // ncurses initialisation -----------------------------------------------------------------------------
                box(my_win, 0, 0);
                wrefresh(my_win);
                keypad(my_win, true);
                message_win = newwin(10, WINDOW_SIZE, WINDOW_SIZE, 0);
                box(message_win, 0, 0);
                wrefresh(message_win);
                // -----------------------------------------------------------------------------------------------------

                draw_player(my_win, &p1, true);
                wrefresh(message_win);
                wrefresh(my_win);
                break;
            }
            else if (message_from_server.msg_type == 'd')
            { // check if it is a 'disconnect' message
                printf("Error: There are already too many balls in the game\n");
                close(sock_fd);
                exit(0);
            }
        }
    }

    int key = -1;
    // read the key pressed, see if it is a valid key and if it is, move the player. Print the key pressed in the message window
    while (key != 27 && key != 'Q')
    {
        key = wgetch(my_win); // saves the key pressed by the user
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)
        {
            // send a 'ball_movement' message to the server
            message_from_client.msg_type = 'm';
            message_from_client.ch = p1.ch;
            message_from_client.movements[0] = key;

            // send 'ball_movement' message
            sendto(sock_fd, &message_from_client, sizeof(message_client), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));

            while (1) // waits for a reply from the server
            {
                n_bytes = recvfrom(sock_fd, &message_from_server, sizeof(message_server), 0, (struct sockaddr *)&check_server_addr, &check_server_addr_size);

                // Check if the address belongs to the server, and not another client trying to cheat
                if (strcmp(check_server_addr.sun_path, server_addr.sun_path) == 0)
                {
                    break;
                }
            }

            if (n_bytes != sizeof(message_server)) // received an invalid message
            {
                continue;
            }
            else if (message_from_server.msg_type == 's')
            {
                // delete old positions (saved previously in players_data array)
                for (int i = 0; i < number_of_players; i++)
                {
                    draw_player(my_win, &players_data[i], false);
                    draw_health(message_win, &players_data[i], false, i);
                }
                number_of_players = message_from_server.number_of_players;
                // update and save the field_status
                for (int i = 0; i < number_of_players; i++)
                {
                    players_data[i] = message_from_server.field_status[i];
                }
                int j = 0;
                // draw field_status
                for (int i = 0; i < number_of_players; i++)
                {
                    draw_player(my_win, &players_data[i], true);
                    draw_health(message_win, &players_data[i], true, j);
                    if (players_data[i].type == 'h')
                    {
                        j++;
                    }
                }
                wrefresh(message_win);
                wrefresh(my_win);
            }
            else if (message_from_server.msg_type == 'h') // 'health_0' message, the player leaves the game
            {
                endwin();
                printf("Game over!\n");
                fflush(stdout);
                close(sock_fd);
                exit(0);
            }
            else
            {
                printf("ELSE");
                fflush(stdout);
                continue;
            }
        }
        else if (key == 'Q')
        {
            // send disconnection message
            message_from_client.msg_type = 'd'; // message type: disconnect
            sendto(sock_fd, &message_from_client, sizeof(message_client), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            endwin();
            printf("You quited the game!\n");
            fflush(stdout);
            close(sock_fd);
            exit(0);
        }
    }
}