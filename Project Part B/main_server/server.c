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

void draw_health(WINDOW *win, player balls[], int n_players) // draws all the balls' health on a window
{
    // erase all the healths on the window
    for (int i = 1; i < 14; i++)
    { // 1st column
        mvwprintw(win, i, 1, "       ");
    }
    for (int i = 1; i < 14; i++)
    { // 2nd column
        mvwprintw(win, i, 10, "       ");
    }

    // print all the healths
    if (n_players <= 13)
    { // 1st column
        for (int i = 0; i < n_players; i++)
        {
            mvwprintw(win, i + 1, 1, "%c %d", balls[i].ch, balls[i].health);
        }
    }
    else
    {
        for (int i = 0; i < 13; i++)
        { // 1st column
            mvwprintw(win, i + 1, 1, "%c %d", balls[i].ch, balls[i].health);
        }
        for (int i = 13; i < n_players; i++)
        { // 2nd column
            mvwprintw(win, i - 12, 10, "%c %d", balls[i].ch, balls[i].health);
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

void *wait_for_continue_game(void *arg)
{
    message_thread *game_info = (message_thread *)arg;
    message_server message_from_server;
    player field_status[46];

    sleep(10); // after the 10 seconds, if the thread is not canceled, we remove the client from the game

    pthread_mutex_lock(&mutex);

    game_info->socket[game_info->index_client] = -1;      // socket is not being used
    game_info->thread_avail[game_info->index_client] = 0; // the thread is now available

    // find player's position in the array with all the balls
    int player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, game_info->ch);
    // remove the player from the screen
    draw_player(game_info->win, &(game_info->balls_array[player_pos]), false);
    // remove the player from the array
    delete_player(game_info->balls_array, player_pos, &game_info->n_players_human, &game_info->n_players);
    // remove the player from the health screen
    draw_health(game_info->health_win, game_info->balls_array, game_info->n_players_human);
    wrefresh(game_info->win);
    wrefresh(game_info->health_win);

    // send 'field_status' message to all the other clients -------------------------------------------------------------------
    message_from_server.msg_type = 's';
    message_from_server.x = game_info->balls_array[player_pos].x;
    message_from_server.y = game_info->balls_array[player_pos].y;
    message_from_server.ch = game_info->balls_array[player_pos].ch;
    message_from_server.health = game_info->balls_array[player_pos].health;
    message_from_server.number_of_players = game_info->n_players; // total number of players in the game
    create_fieldstatus(field_status, game_info->balls_array, game_info->bots_array, game_info->prizes_array, game_info->n_players_human, game_info->n_players_bot, game_info->n_players_prize);
    for (int i = 0; i < game_info->n_players; i++)
    {
        message_from_server.field_status[i] = field_status[i];
    }
    for (int i = 0; i < 26; i++)
    {
        if (game_info->socket[i] != -1)
        {
            send(game_info->socket[i], &message_from_server, sizeof(message_server), 0); // send message to client with the updates
        }
    }
    close(game_info->socket_client);
    pthread_cancel(game_info->tmp_thread);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *bots_function(void *arg)
{
    message_thread *game_info = (message_thread *)arg;
    message_server message_from_server; // message the server sends to the client
    player field_status[46];            // array that stores all the players in the game
    int xy_before[10][2];               // array that stores the x and y coordinates of the bots before movement

    pthread_mutex_lock(&mutex);
    //  generate the initial position of each bot
    for (int i = 0; i < game_info->n_players_bot; i++)
    {
        game_info->bots_array[i] = generate_position(game_info->balls_array, game_info->bots_array, game_info->prizes_array); // generates a bot
        game_info->bots_array[i].ch = '*';
        game_info->bots_array[i].health = -1;
        game_info->bots_array[i].type = 'b';
        draw_player(game_info->win, &(game_info->bots_array[i]), true); // draws the bot on the screen
    }
    wrefresh(game_info->win);
    pthread_mutex_unlock(&mutex);

    cursor_movement movement;
    srand(clock());
    while (1)
    {
        sleep(3); // 3 seconds timer
        pthread_mutex_lock(&mutex);
        srand(clock());
        for (int i = 0; i < game_info->n_players_bot; i++)
        {
            // generate a random movement (up, down, left, right)
            movement = (rand() % 4) + 258; // add 258 due to the cursor keys numbers

            // save bots positions before movement
            xy_before[i][0] = game_info->bots_array[i].x;
            xy_before[i][1] = game_info->bots_array[i].y;

            // delete mark from old position
            draw_player(game_info->win, &(game_info->bots_array[i]), false); // erase bot's old position
            move_player(&(game_info->bots_array[i]), movement);              // update the bot's position according to the movement

            // check if bot rammed into other player----------------------------------------------------------
            // check if it rammed into a ball
            for (int j = 0; j < game_info->n_players_human; j++)
            { // run through all the ball players in the game
                if ((game_info->bots_array[i].x == game_info->balls_array[j].x) && (game_info->bots_array[i].y == game_info->balls_array[j].y))
                { // bot rammed into a ball
                    // bot doesn't move
                    game_info->bots_array[i].x = xy_before[i][0];
                    game_info->bots_array[i].y = xy_before[i][1];
                    if (game_info->balls_array[j].health > 0) // only decrease the health of a ball that's still in the game
                    {
                        game_info->balls_array[j].health--; // ball that was rammed
                    }
                }
            }
            // check if it rammed into a bot
            for (int j = 0; j < game_info->n_players_bot; j++)
            { // run through all the bots in the game
                if ((i != j) && (game_info->bots_array[i].x == game_info->bots_array[j].x) && (game_info->bots_array[i].y == game_info->bots_array[j].y))
                { // bot rammed into another bot
                    game_info->bots_array[i].x = xy_before[i][0];
                    game_info->bots_array[i].y = xy_before[i][1];
                }
            }
            // check if it rammed into a prize
            for (int j = 0; j < game_info->n_players_prize; j++)
            { // run through all the prizes in the game
                if ((game_info->bots_array[i].x == game_info->prizes_array[j].x) && (game_info->bots_array[i].y == game_info->prizes_array[j].y))
                { // bot rammed into a prize
                    // bot doesn't move
                    game_info->bots_array[i].x = xy_before[i][0];
                    game_info->bots_array[i].y = xy_before[i][1];
                }
            }
            draw_player(game_info->win, &(game_info->bots_array[i]), true);
            draw_health(game_info->health_win, game_info->balls_array, game_info->n_players_human);
        }
        wrefresh(game_info->win);

        // sends an update of the bots to the clients --------------------------------------------------------------------
        message_from_server.msg_type = 's';
        message_from_server.number_of_players = game_info->n_players; // total number of players in the game
        create_fieldstatus(field_status, game_info->balls_array, game_info->bots_array, game_info->prizes_array, game_info->n_players_human, game_info->n_players_bot, game_info->n_players_prize);
        for (int i = 0; i < game_info->n_players; i++)
        {
            message_from_server.field_status[i] = field_status[i];
        }
        for (int i = 0; i < 26; i++)
        {
            if (game_info->socket[i] != -1)
            {
                send(game_info->socket[i], &message_from_server, sizeof(message_server), 0); // send message to client with the updates
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *prizes_function(void *arg)
{
    message_thread *game_info = (message_thread *)arg;
    message_server message_from_server; // message the server sends to the client
    player field_status[46];            // array that stores all the players in the game

    srand(clock());
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < 5; i++) // generate the 1st 5 prizes
    {
        game_info->prizes_array[i] = generate_position(game_info->balls_array, game_info->bots_array, game_info->prizes_array); // generates a prize
        int prize_value = rand() % (5 - 1 + 1) + 1;
        game_info->prizes_array[i].ch = prize_value + '0';
        game_info->prizes_array[i].health = prize_value;
        game_info->prizes_array[i].type = 'p';
        draw_player(game_info->win, &(game_info->prizes_array[i]), true); // draws the prize on the screen
        game_info->n_players_prize++;                                     // update the number of prizes in the game
        game_info->n_players++;                                           // update the number of players in the game
    }
    wrefresh(game_info->win);
    pthread_mutex_unlock(&mutex);

    while (1)
    {
        sleep(5); // 5 seconds timer
        pthread_mutex_lock(&mutex);
        if (game_info->n_players_prize < 10) // the game cannnot have more than 10 prizes
        {
            //  generate a prize
            game_info->prizes_array[game_info->n_players_prize] = generate_position(game_info->balls_array, game_info->bots_array, game_info->prizes_array);
            int prize_value = rand() % (5 - 1 + 1) + 1;
            game_info->prizes_array[game_info->n_players_prize].ch = prize_value + '0';
            game_info->prizes_array[game_info->n_players_prize].health = prize_value;
            game_info->prizes_array[game_info->n_players_prize].type = 'p';
            draw_player(game_info->win, &(game_info->prizes_array[game_info->n_players_prize]), true); // draws the new prize on the screen
            game_info->n_players_prize++;                                                              // update the number of prizes in the game
            game_info->n_players++;                                                                    // update the number of players in the game
            wrefresh(game_info->win);

            // sends an update of the prizes to the clients --------------------------------------------------------------------
            message_from_server.msg_type = 's';
            message_from_server.number_of_players = game_info->n_players; // total number of players in the game
            create_fieldstatus(field_status, game_info->balls_array, game_info->bots_array, game_info->prizes_array, game_info->n_players_human, game_info->n_players_bot, game_info->n_players_prize);
            for (int i = 0; i < game_info->n_players; i++)
            {
                message_from_server.field_status[i] = field_status[i];
            }
            for (int i = 0; i < 26; i++)
            {
                if (game_info->socket[i] != -1)
                {
                    send(game_info->socket[i], &message_from_server, sizeof(message_server), 0); // send message to client with the updates
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *clients_function(void *arg)
{
    int n_bytes, player_pos;
    message_thread *game_info = (message_thread *)arg;
    message_client message_from_client; // message the server receives from the client
    message_server message_from_server; // message the server sends to the client
    player field_status[46];            // array that stores all the players in the game
    int socket_client = game_info->socket[game_info->index];
    int self_index = game_info->index;

    while (1)
    {
        n_bytes = recv(socket_client, &message_from_client, sizeof(message_client), 0);
        if (n_bytes == sizeof(message_client)) // message from client was well received
        {
            // deal with the messages sent by the client
            if (message_from_client.msg_type == 'c') // received a 'connection' message, add player to game, store player information
            {
                pthread_mutex_lock(&mutex);
                //  generates a ball in the game
                game_info->balls_array[game_info->n_players_human] = generate_position(game_info->balls_array, game_info->bots_array, game_info->prizes_array);
                game_info->balls_array[game_info->n_players_human].ch = generate_letter(game_info->balls_array, game_info->n_players_human);
                game_info->balls_array[game_info->n_players_human].health = 10; // ball's initial health is 10
                game_info->balls_array[game_info->n_players_human].type = 'h';
                game_info->n_players_human++;
                game_info->n_players++;

                // send a 'ball_information' message ---------------------------------------------------------------------
                message_from_server.msg_type = 'b';
                message_from_server.x = game_info->balls_array[game_info->n_players_human - 1].x;
                message_from_server.y = game_info->balls_array[game_info->n_players_human - 1].y;
                message_from_server.ch = game_info->balls_array[game_info->n_players_human - 1].ch;
                message_from_server.health = 10;
                message_from_server.number_of_players = game_info->n_players;

                send(socket_client, &message_from_server, sizeof(message_server), 0);
                draw_player(game_info->win, &(game_info->balls_array[game_info->n_players_human - 1]), true); // draws the player on the screen
                draw_health(game_info->health_win, game_info->balls_array, game_info->n_players_human);
                wrefresh(game_info->win);
                pthread_mutex_unlock(&mutex);
            }
            else if (message_from_client.msg_type == 'm') // received a 'ball_movement' message, update player's position and reply with field_status
            {
                pthread_mutex_lock(&mutex);
                //  find player's index in the array
                player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, message_from_client.ch);
                // save the player's position before movement
                int x_before = game_info->balls_array[player_pos].x;
                int y_before = game_info->balls_array[player_pos].y;
                int flag_continue_game = 0;

                if (game_info->balls_array[player_pos].health == 0) // if the player's health is 0
                {
                    message_from_server.msg_type = 'h'; // 'health_0' message to client
                    send(socket_client, &message_from_server, sizeof(message_from_server), 0);

                    game_info->socket_client = socket_client;
                    game_info->index_client = self_index;
                    game_info->ch = message_from_client.ch;
                    game_info->tmp_thread = pthread_self();
                    pthread_t thread_counter;

                    pthread_mutex_unlock(&mutex);

                    pthread_create(&thread_counter, NULL, wait_for_continue_game, game_info); // create the thread that removes the player is it doesn't want to continue

                    n_bytes = recv(socket_client, &message_from_client, sizeof(message_client), 0);
                    if (n_bytes == sizeof(message_client))
                    {
                        if (message_from_client.msg_type == 'g') // 'continue game' message
                        {
                            pthread_cancel(thread_counter);
                            pthread_mutex_lock(&mutex);
                            game_info->balls_array[player_pos].health = 10;
                            flag_continue_game = 1;
                        }
                    }
                }

                if (flag_continue_game != 1)
                {
                    player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, message_from_client.ch);

                    if (player_pos != -1)
                    {
                        // delete mark from old position
                        draw_player(game_info->win, &game_info->balls_array[player_pos], false);        // delete player's old position
                        move_player(&game_info->balls_array[player_pos], message_from_client.movement); // update the player's position according to the movement

                        // check if ball rammed into other player----------------------------------------------------------
                        // check if it rammed into a ball
                        for (int i = 0; i < game_info->n_players_human; i++)
                        { // run through all the ball players in the game
                            if ((player_pos != i) && (game_info->balls_array[player_pos].x == game_info->balls_array[i].x) && (game_info->balls_array[player_pos].y == game_info->balls_array[i].y))
                            { // ball rammed into another ball
                                if (game_info->balls_array[player_pos].health < 10 && game_info->balls_array[i].health > 0)
                                {
                                    game_info->balls_array[player_pos].health++; // ball that rammed into an active ball
                                }
                                if (game_info->balls_array[i].health > 0) // only decrease the health of a ball that's still in the game
                                {
                                    game_info->balls_array[i].health--; // ball that was rammed
                                }
                                // ball does not move
                                player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, message_from_client.ch);
                                game_info->balls_array[player_pos].x = x_before;
                                game_info->balls_array[player_pos].y = y_before;
                            }
                        }
                        // check if it rammed into a bot
                        for (int i = 0; i < game_info->n_players_bot; i++)
                        { // run through all the bots in the game
                            if ((game_info->balls_array[player_pos].x == game_info->bots_array[i].x) && (game_info->balls_array[player_pos].y == game_info->bots_array[i].y))
                            {
                                // ball does not move
                                game_info->balls_array[player_pos].x = x_before;
                                game_info->balls_array[player_pos].y = y_before;
                            }
                        }
                        // check if it rammed into a prize
                        for (int i = 0; i < game_info->n_players_prize; i++)
                        {
                            if ((game_info->balls_array[player_pos].x == game_info->prizes_array[i].x) && (game_info->balls_array[player_pos].y == game_info->prizes_array[i].y))
                            { // ball rammed into a prize
                                if (game_info->balls_array[player_pos].health + game_info->prizes_array[i].health <= 10)
                                {                                                                                                                              // a ball's health is never higher than 10
                                    game_info->balls_array[player_pos].health = game_info->balls_array[player_pos].health + game_info->prizes_array[i].health; // a ball's health increases by the value of the prize
                                }
                                else
                                {
                                    game_info->balls_array[player_pos].health = 10;
                                }
                                delete_player(game_info->prizes_array, i, &(game_info->n_players_prize), &(game_info->n_players));
                            }
                        }

                        // draw on window
                        player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, message_from_client.ch);
                        draw_player(game_info->win, &game_info->balls_array[player_pos], true); // draw player's current position
                        draw_health(game_info->health_win, game_info->balls_array, game_info->n_players_human);
                        wrefresh(game_info->win);
                        wrefresh(game_info->health_win);
                    }
                }
                //  send 'field_status' message to all the other clients
                message_from_server.msg_type = 's';
                message_from_server.x = game_info->balls_array[player_pos].x;
                message_from_server.y = game_info->balls_array[player_pos].y;
                message_from_server.ch = game_info->balls_array[player_pos].ch;
                message_from_server.health = game_info->balls_array[player_pos].health;
                message_from_server.number_of_players = game_info->n_players; // total number of players in the game
                create_fieldstatus(field_status, game_info->balls_array, game_info->bots_array, game_info->prizes_array, game_info->n_players_human, game_info->n_players_bot, game_info->n_players_prize);
                for (int i = 0; i < game_info->n_players; i++)
                {
                    message_from_server.field_status[i] = field_status[i];
                }
                for (int i = 0; i < 26; i++)
                {
                    if (game_info->socket[i] != -1)
                    {
                        send(game_info->socket[i], &message_from_server, sizeof(message_server), 0); // send message to client with the updates
                    }
                }
                pthread_mutex_unlock(&mutex);
                continue;
            }
        }
        else if (n_bytes == 0) // the socket of the client closed
        {
            pthread_mutex_lock(&mutex);
            game_info->socket[self_index] = -1;      // socket is not being used
            game_info->thread_avail[self_index] = 0; // the thread is now available

            // find player's position in the array with all the balls
            player_pos = find_ch_info(game_info->balls_array, game_info->n_players_human, message_from_client.ch);
            // remove the player from the screen
            draw_player(game_info->win, &(game_info->balls_array[player_pos]), false);
            // remove the player from the array
            delete_player(game_info->balls_array, player_pos, &(game_info->n_players_human), &(game_info->n_players));
            // remove the player from the health screen
            draw_health(game_info->health_win, game_info->balls_array, game_info->n_players_human);
            wrefresh(game_info->win);
            wrefresh(game_info->health_win);

            // send 'field_status' message to all the other clients -------------------------------------------------------------------
            message_from_server.msg_type = 's';
            message_from_server.x = game_info->balls_array[player_pos].x;
            message_from_server.y = game_info->balls_array[player_pos].y;
            message_from_server.ch = game_info->balls_array[player_pos].ch;
            message_from_server.health = game_info->balls_array[player_pos].health;
            message_from_server.number_of_players = game_info->n_players; // total number of players in the game
            create_fieldstatus(field_status, game_info->balls_array, game_info->bots_array, game_info->prizes_array, game_info->n_players_human, game_info->n_players_bot, game_info->n_players_prize);
            for (int i = 0; i < game_info->n_players; i++)
            {
                message_from_server.field_status[i] = field_status[i];
            }
            for (int i = 0; i < 26; i++)
            {
                if (game_info->socket[i] != -1)
                {
                    send(game_info->socket[i], &message_from_server, sizeof(message_server), 0); // send message to client with the updates
                }
            }
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) // server's address and port
{
    // create socket ------------------------------------------------------------------
    int socket_server, socket_client;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    char line[10];              // buffer to read from the stdin
    int n_bots = 0;             // number of bots read from stdin
    int thread_avail[26] = {0}; // array with the availability of each client thread
    int flag_thread_avail = 1;  // flag which indicates if there is any thread available for the client

    message_server message_to_client; // message the server sends to the client
    message_thread message_thread;    // argument sent to the threads
    pthread_t thread_1;               // thread for the bots
    pthread_t thread_2;               // thread for the prizes
    pthread_t t_id[26];               // array that stores the threads of the clients

    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server == -1)
    {
        perror("socket: ");
        exit(-1);
    }
    int port = atoi(argv[2]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(-1);
    }

    if (listen(socket_server, 1) == -1)
    {
        perror("listen");
        exit(-1);
    }

    // read the number of bots given in server's stdin
    do
    {
        printf("Number of bots (< 10): ");
        fgets(line, 10, stdin);
        sscanf(line, "%d", &n_bots);

    } while (n_bots < 1 || n_bots > 10); // the number of bots must be between 1 and 10

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
    WINDOW *health_win = newwin(15, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(health_win, 0, 0);
    wrefresh(health_win);
    // ----------------------------------------------------------------------------------------------------------------------

    // initialisation of the thread arguments
    message_thread.n_players = n_bots;
    message_thread.n_players_human = 0;
    message_thread.n_players_bot = n_bots;
    message_thread.n_players_prize = 0;
    message_thread.win = win;
    message_thread.health_win = health_win;

    for (int i = 0; i < 26; i++) // save the thread_avail array as an argument for the threads
    {
        message_thread.thread_avail[i] = thread_avail[i];
        message_thread.socket[i] = -1;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_create(&thread_1, NULL, bots_function, &message_thread);   // thread that deals with bots
    pthread_create(&thread_2, NULL, prizes_function, &message_thread); // thread that deals with prizes

    while (1)
    {
        // Accept an incoming connection
        socket_client = accept(socket_server, (struct sockaddr *)&client_addr, &client_size);

        if (socket_client > -1)
        {
            for (int i = 0; i < 26; i++) // checks if there is any thread available
            {
                if (thread_avail[i] == 0) // verify if the thread is available to be used
                {
                    flag_thread_avail = 0;
                    thread_avail[i] = 1;
                    message_thread.thread_avail[i] = 1;
                    message_thread.socket[i] = socket_client; // save the socket of the new client
                    message_thread.index = i;
                    pthread_create(&t_id[i], NULL, clients_function, &message_thread);
                    break;
                }
            }
            if (flag_thread_avail == 1) // all threads are being used
            {
                // server informs the client it cannot enter the game since there are no threads available
                message_to_client.msg_type = 'd'; // send a 'disconnect' message
                if (send(socket_client, &message_to_client, sizeof(message_server), 0) < 0)
                {
                    perror("send");
                    exit(-1);
                }
            }
        }
    }
    pthread_mutex_destroy(&mutex);
}