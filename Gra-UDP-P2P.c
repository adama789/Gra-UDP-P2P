#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

void swap(char *str1, char *str2) { 
    char *temp = (char *)malloc((strlen(str1) + 1) * sizeof(char)); 
    strcpy(temp, str1); 
    strcpy(str1, str2); 
    strcpy(str2, temp); 
    free(temp); 
}

void swap_int(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

struct player {
    char nick1[255];
    char nick2[255];
    int kogo_tura;
    int wynik_pl1;
    int wynik_pl2;
    int punkty;
    char komenda[255];
    int poczatek;
};


int main(int argc, char **argv) {
    srand(time(NULL));

    // Sprawdzanie poprawności argumentów
    if (argc < 3) {
        fprintf(stderr, "Użycie: %s <adres IP lub domena> <port> [nick]\n", argv[0]);
        exit(1);
    }

    char nick_lokalnie[255];
    struct sockaddr_in server_addr, client_addr;
    struct player player;
    int socket_listen, port = atoi(argv[2]);
    socklen_t size = sizeof(server_addr);

    // Otwieranie gniazda na socket_listen
    if ((socket_listen = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Blad przy otwieraniu gniazda");
        exit(1);
    }

    // Inicjalizacja wartości serwera
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    strcpy(player.nick1, "");

    // Wiązanie gniazda na socket_listen
    if (bind(socket_listen, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_listen);
        perror("Blad przy wiazaniu gniazda");
        exit(1);
    }

    // Inicjalizacja wartości klienta
    bzero(&client_addr, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    struct hostent *host;
    if ((host = gethostbyname(argv[1])) == NULL) {
        close(socket_listen);
        perror("Blad pobierania informacji o adresie hosta");
        exit(1);
    }
    memcpy(&client_addr.sin_addr, host->h_addr_list[0], host->h_length);
    client_addr.sin_port = htons(port);

    // Przypisanie nicków
    if (argc == 4) {
        strncpy(player.nick1, argv[3], sizeof(player.nick1) - 1);
        player.nick1[sizeof(player.nick1) - 1] = '\0';
        strncpy(nick_lokalnie, argv[3], sizeof(nick_lokalnie) - 1);
        nick_lokalnie[sizeof(nick_lokalnie) - 1] = '\0';
    } else {
        strcpy(player.nick1, "");
        strcpy(nick_lokalnie, "");
    }

    printf("Gra w 50, wersja B.\n");
    printf("Rozpoczynam gre z %s. Napisz \"koniec\" by zakonczyc lub \"wynik\" by wyswietlic aktualny wynik.\n", inet_ntoa(client_addr.sin_addr));
    printf("Propozycja gry wyslana\n");

    int receive = -1;

    // Inicjalizacja nowej gry
    player.kogo_tura = 1;
    player.poczatek = 1;
    player.punkty = 0;
    player.wynik_pl1 = 0;
    player.wynik_pl2 = 0;

    // Wysyłanie datagramów do Gracza, niniejszym dołączanie graczy do gry 
    if (sendto(socket_listen, &player, sizeof(player), 0, (struct sockaddr*)&client_addr, size) < 0) { 
        close(socket_listen);                           
        perror("Sendto"); 
        exit(1);                                                                                                 
    }

    // Główna pętla gry
    while (1) {
        // Gracz 1, oczekuje
        if (player.kogo_tura == 1) {                    
            if ((receive = recvfrom(socket_listen, &player, sizeof(player), 0, (struct sockaddr *)&client_addr, &size)) < 0) {
                perror("Blad odbierania");
                close(socket_listen);
                exit(1);
            }
            
            receive = -1;

            // Warunki pomocnicze do przypisania nicków
            if (player.poczatek == 2) {
                if (strcmp(player.nick1, "") == 0) {
                    strcpy(player.nick1, inet_ntoa(client_addr.sin_addr));
                    player.poczatek = 0;
                }
            }
            if (player.poczatek == 1) {
                strcpy(player.nick2, nick_lokalnie);
                if (strcmp(player.nick1, "") == 0) {
                    strcpy(player.nick1, inet_ntoa(client_addr.sin_addr));
                }
                printf("%s dolaczyl do gry\n", player.nick1);
            }

            player.kogo_tura = 0;

            // Przeciwnik zakończył grę
            if (strcmp(player.komenda, "koniec") == 0) {
                printf("Gracz %s zakonczyl gre, mozesz poczekac na kolejnego gracza\n", player.nick1);
                strcpy(player.komenda, "");
                strcpy(player.nick2, "");
                player.kogo_tura = 1;
                continue;
            }

            // Przeciwnik wygrał rundę
            if (player.punkty == 50) {
                printf("%s podal wartosc 50.\n", player.nick1);
                printf("Przegrana!\n");
                player.wynik_pl1++;
                player.punkty = 0;
                player.poczatek = 1;
            }
        }

        // Gracz 2, robi ruch
        if (player.kogo_tura == 0) {
            // Początek gry
            if (player.poczatek == 1) {
                int r = (rand() % 10) + 1;
                printf("Losowa wartosc poczatkowa: %d, podaj kolejna wartosc. \n", r);
                printf("> ");
                player.punkty += r;
                player.poczatek = 2;
            } else {
                printf("%s podal wartosc %d, podaj kolejna wartosc: \n", player.nick1, atoi(player.komenda));
                printf("> ");
            }

            // Gracz wykonuje ture
            while (1==1) {
                scanf("%s", player.komenda);
                int wprowadzona_liczba = atoi(player.komenda);
                if (strcmp(player.komenda, "koniec") == 0) {
                    break;
                } else if (strcmp(player.komenda, "wynik") == 0) {
                    printf("Ty %d : %d %s\n", player.wynik_pl2, player.wynik_pl1, player.nick1);
                    printf("> ");
                } else if (wprowadzona_liczba <= player.punkty + 10 && wprowadzona_liczba > player.punkty && wprowadzona_liczba <= 50) {
                    player.punkty = wprowadzona_liczba;
                    break;
                } else {
                    printf("Takiej wartosci nie mozesz wybrac!\n");
                    printf("> ");
                }
                
            }
            
            // Zamiana wartości graczy
            swap(player.nick1, player.nick2);
            swap_int(&player.wynik_pl1, &player.wynik_pl2);

            // Wysłanie struktury do Gracza oczekującego
            if (sendto(socket_listen, &player, sizeof(player), 0, (struct sockaddr*)&client_addr, size) < 0) {
                close(socket_listen);                          
                perror("Sendto");     
                exit(1);                                                                                             
            }

            // Gracz zakończył grę
            if (strcmp(player.komenda, "koniec") == 0) {
                printf("koniec\n");
                break;
            }

            // Gracz wygrał rundę
            if (player.punkty == 50) {
                printf("Wygrana!\n");
                printf("Rozpoczynamy kolejna rozgrywke.\n");
            }

            receive = -1;
            player.kogo_tura = 1;
        }
    }

    close(socket_listen);
    return 0;
}
