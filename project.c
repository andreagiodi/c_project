#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// oi vale, io sono arrivato a questo punto, cosi è una roba che funziona ma bisogna capire se è quello che volevamo fare effettivamente

#define GRID_SIZE 10
#define PORT 12345

char grid1[GRID_SIZE][GRID_SIZE];
char grid2[GRID_SIZE][GRID_SIZE];
int ships_remaining = 5;
int ship_placement_done[2] = {0, 0};

typedef struct {
    int client_socket;
    int player_id;
    char (*grid)[GRID_SIZE];
} ThreadArgs;

void initialize_grid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

void print_grid() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid1[i][j]);
        }
        printf("\n");
    }
}

int check_winner() {
    return ships_remaining == 0;
}

void update_grid(char grid[GRID_SIZE][GRID_SIZE], int x, int y, char result) {
    grid[x][y] = result;
    if (result == 'H') {
        ships_remaining--;
    }
}

void send_instructions(int client_socket) {
    char *instructions = "Welcome to Battleship!\n"
                         "The grid is 10x10. You will place your ships first.\n"
                         "Enter coordinates in the format 'x y' to place your ships.\n";
    write(client_socket, instructions, strlen(instructions));
}

void print_grid_to_client(int client_socket, char grid[GRID_SIZE][GRID_SIZE]) {
    char buffer[1024];
    for (int i = 0; i < GRID_SIZE; i++) {
        int len = 0;
        for (int j = 0; j < GRID_SIZE; j++) {
            len += snprintf(buffer + len, sizeof(buffer) - len, "%c ", grid[i][j]);
        }
        len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
        write(client_socket, buffer, len);
    }
}

void print_opponent_grid_to_client(int client_socket, char grid[GRID_SIZE][GRID_SIZE]) {
    char buffer[1024];
    for (int i = 0; i < GRID_SIZE; i++) {
        int len = 0;
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 'M' || grid[i][j] == 'H') {
                len += snprintf(buffer + len, sizeof(buffer) - len, "%c ", grid[i][j]);
            } else {
                len += snprintf(buffer + len, sizeof(buffer) - len, "~ ");
            }
        }
        len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
        write(client_socket, buffer, len);
    }
    write(client_socket, "\n", 1);
}

void place_ships(int client_socket, char grid[GRID_SIZE][GRID_SIZE]) {
    char buffer[1024];
    int x, y, n;
    for (int i = 0; i < 5; i++) {
        write(client_socket, "Enter coordinates to place your ship: ", 39);
        n = read(client_socket, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            printf("Player disconnected during ship placement\n");
            close(client_socket);
            exit(1);
        }
        buffer[n] = '\0';
        sscanf(buffer, "%d %d", &x, &y);
        x--; 
        y--;
        if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE && grid[x][y] == '~') {
            grid[x][y] = 'S';
            write(client_socket, "Ship placed\n", 12);
            print_grid_to_client(client_socket, grid);
        } else {
            write(client_socket, "Invalid coordinates or position already occupied\n", 49);
            i--;
        }
    }
}

void* place_ships_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    place_ships(args->client_socket, args->grid);
    ship_placement_done[args->player_id] = 1;
    if (!ship_placement_done[1 - args->player_id]) {
        write(args->client_socket, "Waiting for the other player to finish...\n", 43);
    }
    pthread_exit(NULL);
}

void handle_client(int client_socket1, int client_socket2) {
    write(client_socket1, "Waiting for your opponent...\n", 29);
    write(client_socket2, "The game is starting!\n", 22);

    send_instructions(client_socket1);
    send_instructions(client_socket2);

    print_grid_to_client(client_socket1, grid1);
    print_grid_to_client(client_socket2, grid2);

    pthread_t t1, t2;
    ThreadArgs args1 = {client_socket1, 0, grid1};
    ThreadArgs args2 = {client_socket2, 1, grid2};

    pthread_create(&t1, NULL, place_ships_thread, &args1);
    pthread_create(&t2, NULL, place_ships_thread, &args2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    write(client_socket1, "All ships placed. The game has started!\n", 40);
    write(client_socket2, "All ships placed. The game has started!\n", 40);
    int current_player = 1;
    int x, y;
    char buffer[1024];
    int n;

    while (1) {
        if (current_player == 1) {
            write(client_socket1, "It's your turn, Player 1\n", 25);
            write(client_socket2, "Waiting for Player 1...\n", 24);
        } else {
            write(client_socket2, "It's your turn, Player 2\n", 25);
            write(client_socket1, "Waiting for Player 2...\n", 24);
        }

        int client_socket = (current_player == 1) ? client_socket1 : client_socket2;
        char (*current_grid)[GRID_SIZE] = (current_player == 1) ? grid2 : grid1; // Target opponent's grid
        n = read(client_socket, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            printf("Player %d disconnected\n", current_player);
            close(client_socket);
            break;
        }
        buffer[n] = '\0';
        sscanf(buffer, "%d %d", &x, &y);
        x--;  
        y--; 

        if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
            write(client_socket, "Invalid coordinates\n", 20);
            continue;
        }

        if (current_grid[x][y] == '~') {
            update_grid(current_grid, x, y, 'M');
            write(client_socket, "Miss\n", 5);
        } else if (current_grid[x][y] == 'S') {
            update_grid(current_grid, x, y, 'H');
            write(client_socket, "Hit\n", 4);
        } else {
            write(client_socket, "Already targeted\n", 17);
        }

        print_opponent_grid_to_client(client_socket1, grid2);
        print_opponent_grid_to_client(client_socket2, grid1);
        write(client_socket1, "-------------------------\n", 26);
        write(client_socket2, "-------------------------\n", 26);

        if (check_winner()) {
            write(client_socket, "You win!\n", 9);
            break;
        }

        current_player = (current_player == 1) ? 2 : 1;
    }
}

int main() {
    int server_socket, client_socket1, client_socket2;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 2);

    printf("Waiting for players to connect...\n");
    client_socket1 = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    printf("Player 1 connected\n");
    client_socket2 = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    printf("Player 2 connected\n");

    initialize_grid(grid1);
    initialize_grid(grid2);
    handle_client(client_socket1, client_socket2);

    close(server_socket);
    return 0;
}