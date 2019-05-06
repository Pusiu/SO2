// SO2 IS1 222B LAB08
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab08.server.c -o lab08s
// ./lab08s -p 8003

#define _GNU_SOURCE
#define DEBUG_MODE 0
#define MAX_CLIENTS 16

#define MESSAGE_TYPE_GET_CLIENTS 0
#define MESSAGE_TYPE_SEND_MESSAGE 1
#define MESSAGE_TYPE_RECEIVE_MESSAGE 2
#define MESSAGE_TYPE_SET_ID 3
#define MESSAGE_TYPE_PING 4
#define MESSAGE_TYPE_SET_NAME 5
#define MESSAGE_TYPE_DISCONNECT 6
#define MAX_CONTENT_LENGTH 1024+(7) //7 additional characters for commmand

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


int s = -1; //socket
int highestFD = -1;
fd_set connSet;

#pragma pack(1)
typedef struct Message
{
    int senderID;
    char senderName[256];
    int messageType;
    char content[MAX_CONTENT_LENGTH];
} Message;
#pragma pack(0)

typedef struct Client {
    int clientID;
    char name[256];
} Client;



Client** connectedClients;

void SetHighestFD()
{
    highestFD = s;
    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i]==NULL)
            continue;

        if (connectedClients[i]->clientID > highestFD)
            highestFD=connectedClients[i]->clientID;
    }
}

void SendToAll(char* content)
{
    Message m;
    m.senderID=0;
    strcpy(m.senderName, "Serwer");
    m.messageType = MESSAGE_TYPE_RECEIVE_MESSAGE;
    strcpy(m.content, "0 ");
    strcat(m.content, content);
    printf("Wysylam wiadomosc do wszystkich: %s\n", m.content);

    for (int i=0; i < MAX_CLIENTS;i++)
    {
        if (connectedClients[i]==NULL)
            continue;

        send(connectedClients[i]->clientID, &m, sizeof(Message), 0);
    }
}

void VerifyClients()
{
    fd_set set;
    FD_ZERO(&set);
    struct timeval time;
    time.tv_sec=1;
    printf("Odswiezam klientow\n");

    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i]==NULL)
            continue;

        time.tv_sec=1;
        FD_ZERO(&set);
        FD_SET(connectedClients[i]->clientID, &set);

        Message m;
        m.senderID=0;
        m.messageType=MESSAGE_TYPE_PING;
        sprintf(m.content, "%d", connectedClients[i]->clientID);
        if (send(connectedClients[i]->clientID, &m, sizeof(Message), 0) == -1)
        {
            printf("Nie udalo sie wyslac pinga do %d: %s", connectedClients[i]->clientID, strerror(errno));
        }
        int k = select(connectedClients[i]->clientID+1, &set, NULL, NULL, &time);
        int l = recv(connectedClients[i]->clientID, &m, sizeof(Message), 0);
        if (l == -1)
        {
            printf("Wystapil problem z recv() - %s\n", strerror(errno));
        }

        if (!FD_ISSET(connectedClients[i]->clientID, &set) || l==0)
        {
            printf("\tBrak odpowiedzi od %d. Uniewazniam poleczenie.\n", connectedClients[i]->clientID);

            char mess[1024];
            sprintf(mess, "Uzytwonik %s [ID:%d] wychodzi.\n", connectedClients[i]->name, connectedClients[i]->clientID);

            shutdown(connectedClients[i]->clientID, SHUT_RDWR);
            close(connectedClients[i]->clientID);
            free(connectedClients[i]);
            connectedClients[i]=NULL;


            SendToAll(mess);
        }
        else
        {
            printf("\tOdpowiedz od %d.\n", connectedClients[i]->clientID);
        }
    }
    SetHighestFD();
}

void AddClient(int clientID)
{
    Client* c = calloc(1,sizeof(Client));
    c->clientID=clientID;
    strcpy(c->name, "NONAME");

    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i]==NULL)
        {
            connectedClients[i]=c;
            break;
        }
    }

    Message m;
    m.senderID=0;
    sprintf(m.content, "%d", clientID);

    send(clientID, &m, sizeof(m), 0);
    recv(clientID, &m, sizeof(Message), 0);
    strcpy(c->name, m.content);
}

Client* GetClientByID(int id)
{
    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i] != NULL && connectedClients[i]->clientID == id)
            return connectedClients[i];
    }
    return NULL;
}

void PrintClients()
{
    printf("Lista klientow:\n");
    for (int i=0; i <MAX_CLIENTS;i++)
    {
        if (connectedClients[i] != NULL)
        {
            printf("\t%s [ID:%d]\n", connectedClients[i]->name, connectedClients[i]->clientID);
        }
    }
}

void SetupSelect()
{
    FD_ZERO(&connSet);
    FD_SET(s, &connSet);

    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i] != NULL)
        {
            FD_SET(connectedClients[i]->clientID, &connSet);
        }
    }
}



void Interrupt(int s)
{
    shutdown(s, SHUT_RDWR);
    close(s);
    for (int i=0; i < MAX_CLIENTS; i++)
    {
        if (connectedClients[i] != NULL)
        {
            shutdown(connectedClients[i]->clientID, SHUT_RDWR);
            close(connectedClients[i]->clientID);
            free(connectedClients[i]);
        }
    }
    free(connectedClients);
    printf("\nZamknieto socket\n");
    exit(0);
}

int main(int argc, char* argv[])
{

    sigset_t iset;
    struct sigaction act;

    sigemptyset(&iset);
    act.sa_handler = &Interrupt;
    act.sa_mask=iset;
    act.sa_flags=0;
    sigaction(SIGINT, &act, NULL);

    char* port = "8003";

    int op=getopt(argc, argv, "qp:");

    if (op == -1)
    {
        printf("Serwer nalezy uruchomic z jedna z dwoch opcji:\n \
                \t-p port - spowoduje uruchomienie serwera na porcie 'port'\n\
                \t-q - spowoduje zakonczenie uruchomionej instancji\n");
        exit(1);
    }

    
    if (op == 'p')
        port=optarg;


    char* pid = calloc(8, sizeof(char));
    char killName[1024];
    sprintf(killName, "pidof -o %d %s", getpid(), argv[0]);
    FILE* cmd = popen(killName, "r");
    strcpy(killName, "");


    if (fgets(pid, 8, cmd) != NULL)
    {
        sprintf(killName, "kill %s", pid);
        system(killName);
        if (op == 'q')
        {
            printf("Pomyslnie zamknieto instancje serwera. Jego PID: %d\n", atoi(pid));
            exit(EXIT_SUCCESS);
        }
        else
            printf("Serwer jest juz uruchomiony. Poprzednia jego instancja zostanie zamknieta.\n");
    }
    else
    {
        if (op == 'q')
        {
            printf("Uzyto opcji '-q' jednak na tej maszynie nie istnieje obecnie uruchomiona instancja serwera.\n");
            exit(EXIT_FAILURE);
        }
    }
    free(pid);
    

    pclose(cmd);

    if (fork() > 0)
    {
        printf("Utworzono proces potomny. Nastepuje oddzielenie od terminala.\n");
        exit(1);
    }

    if (!DEBUG_MODE)
    {
        freopen("/dev/null", "w", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
    }

    setsid();


    connectedClients = calloc(MAX_CLIENTS, sizeof(Client*));


    struct sockaddr_in servaddr;
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(port));
    servaddr.sin_addr.s_addr=htons(INADDR_ANY);


    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        printf("Blad socket(), %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    SetHighestFD();
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (bind(s,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        printf("Blad bind(), %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    listen(s, MAX_CLIENTS);

    struct sockaddr clientaddr;
    int addrlen=sizeof(clientaddr);
    printf("Oczekuje na polaczenie\n");

    Message mess;

    struct timespec t;
    t.tv_sec=0;
    t.tv_nsec=0;

    while (1)
    {
        SetupSelect();

        if (pselect(highestFD+1, &connSet, NULL, NULL, &t, NULL) > 0)
        {
            if (FD_ISSET(s, &connSet))
            {
                int id = accept(s, &clientaddr, &addrlen);
                if (id < 0)
                {
                    printf("Accept error: %s\n", strerror(errno));    
                }

                if (id > highestFD)
                    highestFD=id;

                printf("Client connected: %d\n", id);

                AddClient(id);
                VerifyClients();
                char mess[1024];
                Client* c = GetClientByID(id);
                sprintf(mess, "Dolaczyl nowy uzytkownik: %s [ID:%d]", c->name, c->clientID);
                SendToAll(mess);
                PrintClients();
            }
            for (int i=0; i < MAX_CLIENTS; i++)
            {
                if (connectedClients[i] != NULL &&  FD_ISSET(connectedClients[i]->clientID, &connSet))
                {
                    if (read(connectedClients[i]->clientID, &mess, sizeof(Message)) > 0)
                    {
                        printf ("Nadeszla wiadomosc od %d. Typ wiadomosci: %d. Zawartosc: %s\n", connectedClients[i]->clientID, mess.messageType, mess.content);

                        if (mess.messageType == MESSAGE_TYPE_GET_CLIENTS)
                            {
                            printf("Wysylam liste klientow do klienta %d\n", mess.senderID);
                            VerifyClients();

                            Message clientIDs;
                            strcpy(clientIDs.content, "\0");
                            for (int i=0; i < MAX_CLIENTS; i++)
                            {
                                if (connectedClients[i]==NULL)
                                    continue;

                                char clientID[8];
                                sprintf(clientID, "%d", connectedClients[i]->clientID);
                                strcat(clientIDs.content, clientID);
                                strcat(clientIDs.content, ",");
                                strcat(clientIDs.content, connectedClients[i]->name);

                                //if (i+1 != MAX_CLIENTS && connectedClients[i+1]!= NULL)
                                strcat(clientIDs.content, ",");
                            }
                            //strcat(clientIDs.content, ".");
                            char* x = calloc(1024, sizeof(char));
                            strcpy(x, clientIDs.content);
                            char* lastComma = strrchr(x, ',');
                            *lastComma='.';
                            strcpy(clientIDs.content, x);
                            free(x);
                            

                            printf("\tWyslano liste skladajaca sie z: %s\n", clientIDs.content);
                            send(connectedClients[i]->clientID, &clientIDs, sizeof(clientIDs), 0);
                        }
                        else if (mess.messageType == MESSAGE_TYPE_SEND_MESSAGE)
                        {
                            int id=0;
                            int senderID=mess.senderID;
                            char content[1024];
                            sscanf(mess.content, "%d %1024[^\n]", &id, content);
                            VerifyClients();
                            Client* c = GetClientByID(id);
                            mess.senderID=0;
                            mess.messageType=MESSAGE_TYPE_RECEIVE_MESSAGE;
                            if (c != NULL)
                            {
                                sprintf(mess.content, "%d %s", senderID, content);
                                send(id, &mess, sizeof(Message), 0);
                                printf("Przekazano wiadomosc do %d\n", id);
                            }
                            else
                            {
                                sprintf(mess.content, "%d %s", 0, "Klient o podanym ID nie istnieje");
                                send(senderID, &mess, sizeof(Message),0);

                                printf("Klient nie istnieje\n");
                            }
                            

                        }
                        else if (mess.messageType == MESSAGE_TYPE_SET_NAME)
                        {
                            strcpy(connectedClients[mess.senderID]->name, mess.content);
                            printf("Klient o ID %d jest od teraz znany jako %s\n", mess.senderID, connectedClients[mess.senderID]->name);
                        }
                        else if (mess.messageType == MESSAGE_TYPE_DISCONNECT)
                        {
                            VerifyClients();
                        }
                    }
                }
            }
        }
    }
}