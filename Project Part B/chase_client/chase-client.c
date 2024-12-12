#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../variables.h"

WINDOW *message_win;
player p1;

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
    if (player->type == 'b' || player->type == 'p') // bots and prizes do not print on the health window
    {
        return;
    }
    if (!delete) // delete the line on the window
    {
        mvwprintw(win, line + 1, 1, "         ");
        mvwprintw(win, (line + 1) - 5, 10, "         ");
        return;
    }
    char str[100] = {'\0'};
    sprintf(str, "%c %d", player->ch, player->health);
    if (line <= 12) // draw the 1st 13 balls' health on the left column
    {
        mvwprintw(win, line + 1, 1, "%s", str);
    }
    else // draw the remaining on the right column
    {
        mvwprintw(win, (line + 1) - 5, 10, "%s", str);
    }
}

int find_ch_info(player char_data[], int n_char, int ch) // find the position of a certain char in an array
{
    for (int i = 0; i < n_char; i++)
    {
        if (ch == char_data[i].ch)
        {
            return i;
        }
    }
    return -1;
}

void *server_function(void *arg)
{
    message_thread_client *thread_info = (message_thread_client *)arg; // info used in the main that the thread needs to update
    message_server message_from_server;                                // create struct to receive a message from the server

    // ..................................................................................................................................................
    // store the thread_info values in variables easier to use
    // ..................................................................................................................................................

    int n_bytes;
    // int bytes_left = sizeof(message_server);

    int socket;
    WINDOW *win;
    WINDOW *health_win;
    player players_data[46];
    pthread_mutex_lock(&mutex);
    socket = thread_info->socket_fd;
    win = thread_info->my_win;
    health_win = thread_info->message_win;
    int number_of_players = thread_info->number_of_players;
    for (int i = 0; i < number_of_players; i++)
    {
        players_data[i] = thread_info->players_data[i];
    }
    pthread_mutex_unlock(&mutex);

    while (1)
    {
        n_bytes = recv(socket, &message_from_server, sizeof(message_server), 0);

        if (n_bytes <= 0) // the connection closed or there was an error
        {
            endwin();
            printf("Connection error!\n");
            fflush(stdout);
            close(socket);
            exit(0);
        }

        if (message_from_server.msg_type == 's') // check if it is a 'field_status' message
        {
            // delete old positions (saved previously in players_data array)
            for (int i = 0; i < number_of_players; i++)
            {
                draw_player(win, &players_data[i], false);
                draw_health(health_win, &players_data[i], false, i);
            }

            // update and save the the field_status
            for (int i = 0; i < message_from_server.number_of_players; i++)
            {
                players_data[i] = message_from_server.field_status[i];
            }
            number_of_players = message_from_server.number_of_players;

            int j = 0;
            // draw field_status
            for (int i = 0; i < number_of_players; i++)
            {
                draw_player(win, &players_data[i], true);
                draw_health(health_win, &players_data[i], true, j);
                if (players_data[i].type == 'h')
                {
                    j++;
                }
            }
            pthread_mutex_lock(&mutex);
            // update thread_info
            thread_info->message_win = health_win;
            thread_info->my_win = win;
            thread_info->number_of_players = number_of_players;
            for (int i = 0; i < number_of_players; i++)
            {
                thread_info->players_data[i] = players_data[i];
            }
            thread_info->socket_fd = socket;

            pthread_mutex_unlock(&mutex);

            wrefresh(thread_info->message_win);
            wrefresh(thread_info->my_win);
        }
        else if (message_from_server.msg_type == 'h') // check if it is a 'health_0' message
        {
            mvwprintw(win, 5, 1, "You lost");
            wrefresh(win);
            pthread_mutex_lock(&mutex);
            thread_info->state = 1; // o client is in the "choosing" state
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            continue;
        }
    }
}

void *keyboard_function(void *arg) // the argument is the message the client sends to the server
{
    message_client message_from_client;
    message_thread_client *thread_info = (message_thread_client *)arg;
    int socket_fd;
    socket_fd = thread_info->socket_fd;
    WINDOW *my_win;

    int key = -1; // saves the key pressed by the user
    int n_bytes;

    while (key != 27 && key != 'Q' && key != 'q')
    {
        pthread_mutex_lock(&mutex);
        my_win = thread_info->my_win;
        pthread_mutex_unlock(&mutex);

        int player_pos = find_ch_info(thread_info->players_data, thread_info->number_of_players, message_from_client.ch);

        if ((key == 'c' || key == 'C') && thread_info->players_data[player_pos].health == 0) // continue game
        {
            mvwprintw(my_win, 5, 1, "             ");
            mvwprintw(my_win, 7, 1, "             ");
            wrefresh(my_win);
            //  send 'continue game' message
            message_from_client.msg_type = 'g'; // message type: continue game
            n_bytes = send(socket_fd, &message_from_client, sizeof(message_client), 0);
            pthread_mutex_lock(&mutex);
            thread_info->state = 0; // state: playing (the player did not not quit the game)
            pthread_mutex_unlock(&mutex);
        }

        key = wgetch(my_win);
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)
        {
            if (thread_info->state == 0)
            {
                //  send a 'ball_movement' message to the server
                message_from_client.msg_type = 'm';
                message_from_client.ch = p1.ch;
                message_from_client.movement = key;

                n_bytes = send(socket_fd, &message_from_client, sizeof(message_client), 0);
            }
        }
    }
    endwin();
    printf("You quited the game!\n");
    fflush(stdout);
    close(socket_fd); // the client closes its socket
    exit(0);

    return NULL;
}

int main(int argc, char *argv[]) // arguments: server's address and port
{

    pthread_t thread_keyboard; // thread for listening to the keyboard
    pthread_t thread_server;   // thread for recieving the server messages

    player players_data[46];
    message_client message_from_client;
    message_server message_from_server;
    message_thread_client thread_info;
    int socket_fd;
    struct sockaddr_in server_addr;
    int n_bytes, number_of_players = 0;

    // socket create and verification
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("socket: ");
        exit(-1);
    }

    int port = atoi(argv[2]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(port);

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    WINDOW *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);

    // connect the client socket to server socket
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(-1);
    }
    else
        printf("connected to the server..\n");

    message_from_client.msg_type = 'c'; // send 'connect' message to the server
    printf("sending connect to server: %c\n", message_from_client.msg_type);
    // Send the message to server:
    if (send(socket_fd, &message_from_client, sizeof(message_client), 0) < 0)
    {
        printf("Unable to send message\n");
        exit(-1);
    }
    else
    {
        while (1)
        {
            n_bytes = recv(socket_fd, &message_from_server, sizeof(message_server), 0);
            // Check if the address belongs to the server, and not another client trying to cheat
            if (n_bytes != sizeof(message_server))
            {
                continue;
            }
            if (message_from_server.msg_type == 'b') // check if it is a 'ball_information' message
            {
                // save the field status as only this player, and save the number of players to delete (at first is only one)
                p1.x = message_from_server.x;
                p1.y = message_from_server.y;
                p1.ch = message_from_server.ch;
                p1.health = message_from_server.health;

                number_of_players = 1;
                players_data[0] = p1;

                // ncurses initialisation ------------------------------------------------------------------------------
                box(my_win, 0, 0);
                wrefresh(my_win);
                keypad(my_win, true);
                message_win = newwin(15, WINDOW_SIZE, WINDOW_SIZE, 0);
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
                endwin();
                printf("Error: There are already too many balls in the game\n");
                fflush(stdout);
                close(socket_fd);
                exit(0);
            }
        }
        thread_info.my_win = my_win;
        thread_info.socket_fd = socket_fd;
        thread_info.number_of_players = number_of_players;
        thread_info.message_win = message_win;
        thread_info.state = 0;

        for (int i = 0; i < number_of_players; i++)
        {
            thread_info.players_data[i] = players_data[i];
        }

        pthread_mutex_init(&mutex, NULL);

        pthread_create(&thread_keyboard, NULL, keyboard_function, &thread_info); // thread that listens to the keyboard and sends the messages to the server
        pthread_create(&thread_server, NULL, server_function, &thread_info);     // thread that recieves the messages from the server

        pthread_join(thread_keyboard, NULL);
        pthread_join(thread_server, NULL);

        pthread_mutex_destroy(&mutex);
        exit(0);
    }
}