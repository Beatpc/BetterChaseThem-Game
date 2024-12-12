#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <time.h>

#include "../variables.h"

void create_fieldstatus(player *field, player balls[], player bots[], player prizes[], int n_balls, int n_bots, int n_prizes)
{ // create array with all the players of the game
    for (int i = 0; i < n_balls; i++)
    { // add the balls
        field[i] = balls[i];
    }
    for (int i = n_balls; i < n_bots + n_balls; i++)
    { // add the bots
        field[i] = bots[i - n_balls];
    }
    for (int i = n_bots + n_balls; i < n_prizes + n_bots + n_balls; i++)
    { // add the prizes
        field[i] = prizes[i - n_bots - n_balls];
    }
    return;
}

void delete_player(player *array, int index, int *n_elements, int *n_players)
{ // remove a specific element from an array
    for (int i = index; i < (*n_elements) - 1; i++)
    {
        array[i] = array[i + 1];
    }
    (*n_elements)--; // decrease number of elements in array;
    (*n_players)--;  // decrease total number of players
    return;
}

char generate_letter(player balls_array[], int n_balls)
{ // generates a random letter that has is not being used in the game
    char letter;
    int flag = 0;

    srand(clock());
    while (!flag)
    {

        flag = 1;
        letter = (rand() % (90 - 65 + 1)) + 65; // a letter between 'A' and 'Z' (uppercase letters)

        for (int i = 0; i < n_balls; i++)
        {
            if (balls_array[i].ch == letter)
            {             // check if the letter is already being used
                flag = 0; // must continue looking for an unused letter (continue while())
            }
        }
    }
    return letter;
}

struct player generate_position(player balls_array[], player bots_array[], player prizes_array[]) // generate a random position for a player in the field
{
    int x, y; // initialisation of the players's position
    int flag = 0;
    player generated_position;

    srand(clock());
    while (!flag) // loops until a random position generated of the field is empty (has no players in it)
    {
        flag = 1;
        x = rand() % (18 - 1 + 1) + 1; // initial x coordinate (between 1 and 19)
        y = rand() % (18 - 1 + 1) + 1; // initial y coordinate (between 1 and 19)

        for (int i = 0; i < 10; i++)
        { // check if there is any player occupying the same position
            if (bots_array[i].x == x && bots_array[i].y == y)
            {
                flag = 0;
                break;
            }
            else if (balls_array[i].x == x && balls_array[i].y == y)
            { // compare the coordinates the balls in the game with the random coordinates generated
                flag = 0;
                break; // the position in the field is occupied, so break the loop and generate new coordinates
            }
            else if (prizes_array[i].x == x && prizes_array[i].y == y)
            {
                flag = 0;
                break;
            }
        }
    }
    generated_position.x = x;
    generated_position.y = y;

    return generated_position;
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

void draw_health(WINDOW *win, player balls[], int n_players)
{ // draws all the balls' health on a window
    // erase all the healths on the window
    for (int i = 1; i < 6; i++)
    { // 1st column
        mvwprintw(win, i, 1, "       ");
    }
    for (int i = 1; i < 6; i++)
    { // 2nd column
        mvwprintw(win, i, 10, "       ");
    }

    // print all the healths
    if (n_players <= 5)
    { // 1st column
        for (int i = 0; i < n_players; i++)
        {
            mvwprintw(win, i + 1, 1, "%c %d", balls[i].ch, balls[i].health);
        }
    }
    else
    {
        for (int i = 0; i < 5; i++)
        { // 1st column
            mvwprintw(win, i + 1, 1, "%c %d", balls[i].ch, balls[i].health);
        }
        for (int i = 5; i < n_players; i++)
        { // 2nd column
            mvwprintw(win, i - 4, 10, "%c %d", balls[i].ch, balls[i].health);
        }
    }
    wrefresh(win);
}

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
    wrefresh(win);
}

void move_player(player *player, int direction)
{
    if (direction == KEY_UP)
    {
        if (player->y != 1)
        {
            player->y--;
        }
    }
    if (direction == KEY_DOWN)
    {
        if (player->y != WINDOW_SIZE - 2)
        {
            player->y++;
        }
    }

    if (direction == KEY_LEFT)
    {
        if (player->x != 1)
        {
            player->x--;
        }
    }
    if (direction == KEY_RIGHT)
        if (player->x != WINDOW_SIZE - 2)
        {
            player->x++;
        }
}

int main()
{
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket: ");
        exit(-1);
    }
    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCKET_NAME);
    unlink(SOCKET_NAME);
    int err = bind(sock_fd,
                   (const struct sockaddr *)&local_addr, sizeof(local_addr));
    if (err == -1)
    {
        perror("bind");
        exit(-1);
    }

    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    // ncurses initialisation -----------------------------------------------------------------------------------------------
    initscr();            /* Start curses mode 		*/
    cbreak();             /* Line buffering disabled	*/
    keypad(stdscr, TRUE); /* We get F1, F2 etc..		*/
    noecho();             /* Don't echo() while we do getch */

    // game window
    WINDOW *win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(win, 0, 0);
    wrefresh(win);
    keypad(win, true);
    // health window
    WINDOW *health_win = newwin(10, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(health_win, 0, 0);
    wrefresh(health_win);

    // variables declaration -------------------------------------------------------------------------------------------------
    message_client message_from_client; // message the server receives from a client
    message_server message_from_server; // message the server sends to a client

    player balls_array[10];  // array that stores the balls in the game
    player bots_array[10];   // array that stores the bots in the game
    player prizes_array[10]; // array that stores the prizes in the game
    player field_status[30]; // array that stores all the players in the game

    int n_bytes = 0;

    int n_players = 0;       // total number of players
    int n_players_human = 0; // number of balls
    int n_players_bot = 0;   // number of bots
    int n_players_prize = 0; // number of prizes

    int flag_botclient = 0;   // flag which indicates if there is already a bot client in the game
    int flag_prizeclient = 0; // flag which indicates if there is already a prize client in the game

    int player_pos = -1;  // player's index in array
    int xy_before[10][2]; // array that stores the x and y coordinates of the bots before movement

    while (1) // the server waits for a message
    {
        n_bytes = recvfrom(sock_fd, &message_from_client, sizeof(message_client), 0, (struct sockaddr *)&client_addr, &client_addr_size);

        if (n_bytes != sizeof(message_client)) // the server didn't receive all the information about the player (ignore the message)
        {
            continue;
        }
        else if (message_from_client.msg_type == 'c') // received a 'connection' message, add player to game, store player information
        {
            if (message_from_client.player_type == 'h') // ball (human) player
            {
                if (n_players_human == 10)
                {                                       // the maximum of balls in the game is 10
                    message_from_server.msg_type = 'd'; // the server sends a message to the client to disconnect
                    sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                    continue;
                }
                // generates a ball in the game
                balls_array[n_players_human] = generate_position(balls_array, bots_array, prizes_array);
                balls_array[n_players_human].ch = generate_letter(balls_array, n_players_human);
                balls_array[n_players_human].health = 10; // ball's initial health is 10
                balls_array[n_players_human].type = 'h';
                n_players_human++;
                n_players++;

                // sends a 'ball_information' message
                message_from_server.msg_type = 'b';
                message_from_server.x = balls_array[n_players_human - 1].x;
                message_from_server.y = balls_array[n_players_human - 1].y;
                message_from_server.ch = balls_array[n_players_human - 1].ch;
                message_from_server.health = 10;
                message_from_server.number_of_players = n_players;

                sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                draw_player(win, &balls_array[n_players_human - 1], true); // draws the player on the screen
                draw_health(health_win, balls_array, n_players_human);
                wrefresh(win);
                continue;
            }
            else if (message_from_client.player_type == 'b') // bot player
            {
                if (flag_botclient == 1) // there is already a bot client connected
                {
                    message_from_server.msg_type = 'd'; // the server sends a message to the client to disconnect, since there cannot be more bot clients
                    sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                }
                else // the bot client must connect to the server
                {
                    for (int i = 0; i < message_from_client.n_bots; i++) // the server generates the initial position of each bot
                    {
                        bots_array[i] = generate_position(balls_array, bots_array, prizes_array); // generates a bot
                        bots_array[i].ch = '*';
                        bots_array[i].health = -1;
                        bots_array[i].type = 'b';
                        draw_player(win, &bots_array[i], true); // draws the bot on the screen
                        n_players_bot++;                        // update the number of prizes in the game
                        n_players++;
                    }
                    wrefresh(win);

                    message_from_server.msg_type = 'c'; // the server sends a message to the client to connect
                    sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                    flag_botclient = 1; // flag to 1 since a bot client connected
                }
                continue;
            }
            else if (message_from_client.player_type == 'p') // prize player
            {
                if (flag_prizeclient == 1) // there is already a prize client connected
                {
                    message_from_server.msg_type = 'd'; // the server sends a message to the client to disconnect, since there cannot be more prize clients
                    sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                    continue;
                }
                else // the prize client must connect to the server
                {
                    srand(clock());
                    for (int i = 0; i < 5; i++) // generate the 1st 5 prizes
                    {
                        prizes_array[i] = generate_position(balls_array, bots_array, prizes_array); // generates a prize
                        int prize_value = rand() % (5 - 1 + 1) + 1;
                        prizes_array[i].ch = prize_value + '0';
                        prizes_array[i].health = prize_value;
                        prizes_array[i].type = 'p';
                        draw_player(win, &prizes_array[i], true); // draws the prize on the screen
                        n_players_prize++;                        // update the number of prizes in the game
                        n_players++;
                    }
                    wrefresh(win);

                    message_from_server.msg_type = 'c'; // the server sends a message to the client to connect
                    sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                    flag_prizeclient = 1; // flag to 1 since a prize client connected
                }
                continue;
            }
        }
        else if (message_from_client.msg_type == 'm') // received a 'ball_movement' message, update player's position and reply with field_status
        {
            // find player's index in the array
            player_pos = find_ch_info(balls_array, n_players_human, message_from_client.ch);
            // save the player's position before movement
            int x_before = balls_array[player_pos].x;
            int y_before = balls_array[player_pos].y;

            if (player_pos != -1)
            {
                // delete mark from old position
                draw_player(win, &balls_array[player_pos], false);                       // delete player's old position
                move_player(&balls_array[player_pos], message_from_client.movements[0]); // update the player's position according to the movement

                // check if ball rammed into other player----------------------------------------------------------
                // check if it rammed into a ball
                for (int i = 0; i < n_players_human; i++)
                { // run through all the ball players in the game
                    if ((player_pos != i) && (balls_array[player_pos].x == balls_array[i].x) && (balls_array[player_pos].y == balls_array[i].y))
                    { // ball rammed into another ball
                        if (balls_array[player_pos].health < 10)
                        {                                     // a ball's health is never higher than 10
                            balls_array[player_pos].health++; // ball that rammed
                        }
                        balls_array[i].health--; // ball that was rammed
                        if (balls_array[i].health == 0)
                        {
                            draw_player(win, &balls_array[i], false);
                            delete_player(balls_array, i, &n_players_human, &n_players); // ball that was rammed reached health = 0
                        }
                        // ball does not move
                        player_pos = find_ch_info(balls_array, n_players_human, message_from_client.ch);
                        balls_array[player_pos].x = x_before;
                        balls_array[player_pos].y = y_before;
                    }
                }
                // check if it rammed into a bot
                for (int i = 0; i < n_players_bot; i++)
                { // run through all the bots in the game
                    if ((balls_array[player_pos].x == bots_array[i].x) && (balls_array[player_pos].y == bots_array[i].y))
                    {
                        // ball does not move
                        balls_array[player_pos].x = x_before;
                        balls_array[player_pos].y = y_before;
                    }
                }
                // check if it rammed into a prize
                for (int i = 0; i < n_players_prize; i++)
                {
                    if ((balls_array[player_pos].x == prizes_array[i].x) && (balls_array[player_pos].y == prizes_array[i].y))
                    { // ball rammed into a prize
                        if (balls_array[player_pos].health + prizes_array[i].health <= 10)
                        {                                                                                             // a ball's health is never higher than 10
                            balls_array[player_pos].health = balls_array[player_pos].health + prizes_array[i].health; // a ball's health increases by the value of the prize
                        }
                        delete_player(prizes_array, i, &n_players_prize, &n_players);
                    }
                }

                // draw on window
                player_pos = find_ch_info(balls_array, n_players_human, message_from_client.ch);
                draw_player(win, &balls_array[player_pos], true); // draw player's current position
                draw_health(health_win, balls_array, n_players_human);
                wrefresh(win);
                wrefresh(health_win);
            }
            else // if the player is not in the array, it means is not in the game (must send health_0 message)
            {
                message_from_server.msg_type = 'h'; // 'health_0' message to client
                sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size);
                continue;
            }
            // send 'field_status' message
            message_from_server.msg_type = 's';
            message_from_server.x = balls_array[player_pos].x;
            message_from_server.y = balls_array[player_pos].y;
            message_from_server.ch = balls_array[player_pos].ch;
            message_from_server.health = balls_array[player_pos].health;
            message_from_server.number_of_players = n_players; // total number of players in the game
            create_fieldstatus(field_status, balls_array, bots_array, prizes_array, n_players_human, n_players_bot, n_players_prize);
            for (int i = 0; i < n_players; i++)
            {
                message_from_server.field_status[i] = field_status[i];
            }
            sendto(sock_fd, &message_from_server, sizeof(message_from_server), 0, (const struct sockaddr *)&client_addr, client_addr_size); // send message to client with the updates
            continue;
        }
        else if (message_from_client.msg_type == 'd') // received a 'disconnect' message, remove player from the game
        {
            // find player's position in the array with all the players
            player_pos = find_ch_info(balls_array, n_players_human, message_from_client.ch);
            draw_player(win, &balls_array[player_pos], false);                    // removes the player from the screen
            delete_player(balls_array, player_pos, &n_players_human, &n_players); // removes the player from the array
            wrefresh(win);
            wrefresh(health_win);
            continue;
        }
        else if (message_from_client.msg_type == 'b') // received a 'bots_information' message, update the bots' positions
        {
            for (int i = 0; i < n_players_bot; i++)
            {
                // save bots positions before movement
                xy_before[i][0] = bots_array[i].x;
                xy_before[i][1] = bots_array[i].y;

                // delete mark from old position
                draw_player(win, &bots_array[i], false);                       // erase bot's old position
                move_player(&bots_array[i], message_from_client.movements[i]); // update the bot's position according to the movement

                // check if ball rammed into other player----------------------------------------------------------
                // check if it rammed into a ball
                for (int j = 0; j < n_players_human; j++)
                { // run through all the ball players in the game
                    if ((bots_array[i].x == balls_array[j].x) && (bots_array[i].y == balls_array[j].y))
                    { // bot rammed into a ball
                        // bot doesn't move
                        bots_array[i].x = xy_before[i][0];
                        bots_array[i].y = xy_before[i][1];
                        balls_array[j].health--; // ball that was rammed decreases its health by 1 point
                        if (balls_array[j].health == 0)
                        { // ball that was rammed reached health = 0
                            draw_player(win, &balls_array[j], false);
                            delete_player(balls_array, i, &n_players_human, &n_players); // ball removed from the game
                        }
                    }
                }
                // check if it rammed into a bot
                for (int j = 0; j < n_players_bot; j++)
                { // run through all the bots in the game
                    if ((i != j) && (bots_array[i].x == bots_array[j].x) && (bots_array[i].y == bots_array[j].y))
                    { // bot rammed into another bot
                        bots_array[i].x = xy_before[i][0];
                        bots_array[i].y = xy_before[i][1];
                    }
                }
                // check if it rammed into a prize
                for (int j = 0; j < n_players_prize; j++)
                { // run through all the prizes in the game
                    if ((bots_array[i].x == prizes_array[j].x) && (bots_array[i].y == prizes_array[j].y))
                    { // bot rammed into a prize
                        // bot doesn't move
                        bots_array[i].x = xy_before[i][0];
                        bots_array[i].y = xy_before[i][1];
                    }
                }
                draw_player(win, &bots_array[i], true);
                draw_health(health_win, balls_array, n_players_human);
            }
            wrefresh(win);
        }
        else if (message_from_client.msg_type == 'p') // received a 'prize_information' message, create prizes in the game
        {
            if (n_players_prize < 10) // the game cannnot have more than 10 prizes
            {
                // generate a prize
                prizes_array[n_players_prize] = generate_position(balls_array, bots_array, prizes_array);
                int prize_value = rand() % (5 - 1 + 1) + 1;
                prizes_array[n_players_prize].ch = prize_value + '0';
                prizes_array[n_players_prize].health = prize_value;
                prizes_array[n_players_prize].type = 'p';
                draw_player(win, &prizes_array[n_players_prize], true); // draws the new prize on the screen
                n_players_prize++;                                      // update the number of prizes in the game
                n_players++;                                            // update the number of players in the game
                wrefresh(win);
            }
            continue;
        }
    }
}