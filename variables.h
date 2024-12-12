#define WINDOW_SIZE 20

pthread_mutex_t mutex;

typedef struct player // ball, bot, prize
{
    int x, y;   // coordinates of the player in the game field
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
    char msg_type;    // 'c'- connect,  'm'- ball_movement, 'd'- disconnect
    char player_type; // 'h' - human (ball)
    char ch;
    cursor_movement movement; // array which stores the movement of the ball
} message_client;

typedef struct message_server // message the server sends to the client
{
    char msg_type;           // 'i'- ball_information, 's'- field_status, 'h'- health_0, 'd' - disconnect
    char ch;                 // client's character (identifier) in the game
    int x, y;                // position of the client in the field game
    int health;              // health of the player
    player field_status[30]; // array de estruturas com o field status
    int number_of_players;   // total number of players in the game
} message_server;

typedef struct message_thread // arguments the server sends to the threads
{
    int socket[26];          // array with the sockets of each client
    int thread_avail[26];    // array with the threads available in the game (0- free, 1- occupied)
    int index;               // index in the thread_avail[]/socket[] of the thread/socket of the client entering to the game
    int n_players;           // total number of players
    int n_players_human;     // number of balls
    int n_players_bot;       // number of bots
    int n_players_prize;     // number of prizes
    player balls_array[26];  // array that stores the balls in the game
    player bots_array[10];   // array that stores the bots in the game
    player prizes_array[10]; // array that stores the prizes in the game
    WINDOW *win;             // game window
    WINDOW *health_win;      // healths' window
    int socket_client;
    int index_client;
    int ch;
    pthread_t tmp_thread; // temporary id of the client thread that will be eliminated
} message_thread;

typedef struct message_thread_client // arguments the client sends to the threads
{
    WINDOW *my_win;      // field game window
    WINDOW *message_win; // players' health window
    int socket_fd;
    int number_of_players;   // total number of players in the game
    player players_data[46]; // all players in the game
    int state;               // state: 0 - playing, 1 - in continue_game
} message_thread_client;