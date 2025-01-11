#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 2
#define DIM_GRIGLIA 5

typedef struct {
    int id;
    int socket;
    char griglia[DIM_GRIGLIA][DIM_GRIGLIA];
} Client;

void Inizializza_client(Client *client, int id, int socket) {
    client->id = id;
    client->socket = socket;
    memset(client->griglia, 'O', sizeof(client->griglia));  // 'O' rappresenta una cella vuota
}

void Stampa_griglia(Client *client) {
    printf("\nGriglia del client %d:\n", client->id);
    for (int i = 0; i < DIM_GRIGLIA; i++) {
        for (int j = 0; j < DIM_GRIGLIA; j++) {
            printf("%c ", client->griglia[i][j]);
        }
        printf("\n");
    }
}

void Nascondi_nave(Client *client) {
    int riga, colonna;
    char buffer[256];

    sprintf(buffer, "Client %d, inserisci le coordinate per nascondere la tua nave (riga e colonna, da 0 a %d):\n", client->id, DIM_GRIGLIA - 1);
    send(client->socket, buffer, strlen(buffer), 0);

    recv(client->socket, buffer, sizeof(buffer), 0);
    sscanf(buffer, "%d %d", &riga, &colonna);

    if (riga >= 0 && riga < DIM_GRIGLIA && colonna >= 0 && colonna < DIM_GRIGLIA) {
        client->griglia[riga][colonna] = 'X';
        sprintf(buffer, "La nave è stata posizionata nelle coordinate (%d, %d)\n", riga, colonna);
        send(client->socket, buffer, strlen(buffer), 0);
    } else {
        sprintf(buffer, "Coordinate invalide, riprova.\n");
        send(client->socket, buffer, strlen(buffer), 0);
    }
}

int Indovina_posizione(Client *attaccante, Client *difensore) {
    int riga, colonna;
    char buffer[256];

    while (1) {
        sprintf(buffer, "Client %d, inserisci le coordinate per indovinare la posizione della nave dell'avversario (riga e colonna, da 0 a %d):\n", attaccante->id, DIM_GRIGLIA - 1);
        send(attaccante->socket, buffer, strlen(buffer), 0);

        recv(attaccante->socket, buffer, sizeof(buffer), 0);
        sscanf(buffer, "%d %d", &riga, &colonna);

        if (riga >= 0 && riga < DIM_GRIGLIA && colonna >= 0 && colonna < DIM_GRIGLIA) {
            if (difensore->griglia[riga][colonna] == 'X') {
                sprintf(buffer, "Complimenti! Hai indovinato! La nave dell'avversario è nelle coordinate (%d, %d)\n", riga, colonna);
                send(attaccante->socket, buffer, strlen(buffer), 0);
                sprintf(buffer, "Il client %d ha vinto indovinando la posizione della nave!\n", attaccante->id);
                send(difensore->socket, buffer, strlen(buffer), 0);
                //return 1; // Vittoria

                sprintf(buffer, "Si vuole riavviare la partita?\n");
                send(attaccante->socket, buffer, strlen(buffer), 0);
                send(difensore->socket, buffer, strlen(buffer), 0);

                char rispostaAttaccante[256];
                char rispostaDifensore[256];
                recv(attaccante->socket, rispostaAttaccante, sizeof(rispostaAttaccante), 0);
                recv(difensore->socket, rispostaDifensore, sizeof(rispostaDifensore), 0);
                // rimuovo l'ultimo carattere se no non funziona
                rispostaAttaccante[strlen(rispostaAttaccante)-1] = '\0';
                rispostaDifensore[strlen(rispostaDifensore)-1] = '\0';
                printf("Risposta client %d: %s\n", attaccante->id, rispostaAttaccante);
                printf("Risposta client %d: %s\n", difensore->id, rispostaDifensore);
                if (strcmp(rispostaAttaccante, "n") == 0 || strcmp(rispostaDifensore, "n") == 0) {
                    sprintf(buffer, "Uno dei due giocatori ha deciso di interrompere il gioco.");
                    send(attaccante->socket, buffer, strlen(buffer),0);
                    send(difensore->socket, buffer, strlen(buffer),0);
                    return 1;
                } else if (strcmp(rispostaAttaccante, "s") == 0 && strcmp(rispostaDifensore, "s") == 0) {
                    sprintf(buffer, "La partita è stata riavviata.");
                    send(attaccante->socket, buffer, strlen(buffer),0);
                    send(difensore->socket, buffer, strlen(buffer),0);
                    Inizializza_client(attaccante, attaccante->id, attaccante->socket);
                    Inizializza_client(difensore, difensore->id, difensore->socket);
                    Nascondi_nave(attaccante);
                    Nascondi_nave(difensore);
                    Stampa_griglia(attaccante);
                    Stampa_griglia(difensore);
                    return 0;
                } else {
                    sprintf(buffer, "Risposta non valida, la partita è stata interrotta.");
                    send(attaccante->socket, buffer, strlen(buffer),0);
                    send(difensore->socket, buffer, strlen(buffer),0);
                    return 1;
                }

            } else {
                sprintf(buffer, "Mancato! La nave dell'avversario non è nelle coordinate (%d, %d)\n", riga, colonna);
                send(attaccante->socket, buffer, strlen(buffer), 0);
                return 0; // Continua il gioco
            }
        } else {
            sprintf(buffer, "Coordinate invalide, riprova.\n");
            send(attaccante->socket, buffer, strlen(buffer), 0);
        }
    }
}



int main() {
    Client clients[MAX_CLIENTS];
    int server_fd, client_fd;
    int client1 = 1, client2 = 2;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_count = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    listen(server_fd, MAX_CLIENTS);
    printf("Server listening on port %d\n", PORT);

    while (client_count < MAX_CLIENTS) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        clients[client_count].socket = client_fd;
        client_count++;
        printf("Client %d connected\n", client_count);
    }

    Inizializza_client(&clients[0], client1, clients[0].socket);
    Inizializza_client(&clients[1], client2, clients[1].socket);

    Nascondi_nave(&clients[0]);
    Nascondi_nave(&clients[1]);

    Stampa_griglia(&clients[0]);
    Stampa_griglia(&clients[1]);

    while (1) {
        if (Indovina_posizione(&clients[0], &clients[1])) break;
        if (Indovina_posizione(&clients[1], &clients[0])) break;
    }

    close(server_fd);
    return 0;
}