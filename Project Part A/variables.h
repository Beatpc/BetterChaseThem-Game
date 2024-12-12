// header usado para definir as várias variáveis utilizadas ao longo do código

#define WINDOW_SIZE 20
#define SOCKET_NAME "/tmp/socket_chase"

typedef struct player // ball, bot, prize
{
    int x, y;
    char ch;    // letter of the ball, symbol of the bot, value of the prize
    int health; // health of the player, value of the prize
    char type;  // 'h' - human, 'b' - bot, 'p' - prize
} player;

typedef enum cursor_movement
{
    DOWN, // 0 (cursor key: 258)
    UP,   // 1 (cursor key: 259)
    LEFT, // 2 (cursor key: 260)
    RIGHT // 3 (cursor key: 261)

} cursor_movement;

typedef struct message_client // message the client sends to the server
{
    char msg_type;    // 'c'- connect,  'm'- ball_movement, 'b' - bots_information, 'f' - first_prize_information, 'p' - prize_information, 'd'- disconnect
    char player_type; // 'h' - human (ball), 'b' - bot, 'p' - prize
    char ch;
    cursor_movement movements[10]; // array which stores the movements of balls and bots
    int n_bots;                    // number of bots in the game
} message_client;

typedef struct message_server // message the server sends to the client
{
    char msg_type; // 'c' - connect, 's' - field status, 'b' - ball_information , 'h' - health_0, 'd' - disconnect
    char ch;
    int x, y;
    int health;              // health of the player
    player field_status[30]; // array de estruturas com o field status
    int number_of_players;
} message_server;
