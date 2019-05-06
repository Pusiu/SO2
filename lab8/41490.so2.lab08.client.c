// SO2 IS1 222B LAB08
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab08.main.c -o lab08c
// ./lab08c -a 127.0.0.1 -p 8003 -n Test


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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

int s=-1;
int myID=-1;

#pragma pack(1)
typedef struct Message
{
    int senderID;
    char senderName[256];
    int messageType;
    char content[MAX_CONTENT_LENGTH];
} Message;
#pragma pack(0)

void Interrupt(int k)
{
    Message m;
    m.senderID=myID;
    m.messageType=MESSAGE_TYPE_DISCONNECT;
    send(s, &m,sizeof(Message),0);

    shutdown(s, SHUT_RDWR);
    printf("\nZamknieto socket\n");
    exit(0);
}

void PrintInfo()
{
    printf("Wybierz akcje:\nWpisz 'get' aby wypisac liste uzytkownikow\nWpisz 'send [id] [zawartosc] zeby wyslac wiadomosc do uzytkownika\nWpisz 'exit' aby wyjsc\n");
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

    char* ip = NULL;
    char* port = NULL;

    char* username=NULL;

    if (argc == 1)
    {
        printf("Program nalezy uruchomic z trzema przelacznikami.\n-a [adres ip] - gdzie nalezy podac adres IP (np. 127.0.0.1)\n-p [port] - gdzie nalezy podac port (np. 8003)\n-n [name] - gdzie nalezy podac swoj pseudonim (np. Test)\n");
        exit(EXIT_SUCCESS);
    }

    int op;
    while ((op=getopt(argc, argv, "a:p:n:")) != -1)
    {
            switch (op)
            {
                    case 'a':
                            ip=optarg;
                            break;
                    case 'p':
                            port=optarg; 
                            break;
                    case 'n':
                            username=optarg;
                        break;
            }
    }

    if (ip == NULL)
    {
        printf ("Nie sprecyzowano adresu IP!\n");
        exit(EXIT_FAILURE);
    }
    else if (port == NULL)
    {
        printf ("Nie sprecyzowano portu!\n");
        exit(EXIT_FAILURE);
    }
    else if (username == NULL)
    {
        printf ("Nie podano pseudonimu!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(8003);
    servaddr.sin_addr.s_addr=inet_addr(ip);


    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        printf("Blad socket()");
    }

    if (connect(s, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("Nie udalo sie polaczyc do serwera.\n");
        exit(1);
    }

    printf("Polaczono do serwera, socket: %d.\n", s);

    Message idMess;
    recv(s, &idMess, sizeof(idMess), 0);
    myID=atoi(idMess.content);
    printf("Twoje id to: %d\n", myID);

    strcpy(idMess.content, username);
    idMess.senderID=myID;
    idMess.messageType=MESSAGE_TYPE_SET_NAME;
    send(s, &idMess, sizeof(Message),0);
    printf("Twoj nick to %s\n", username);

    fd_set set;
    struct timeval time;
    time.tv_sec=0;
    time.tv_usec=0;
    PrintInfo();
    while (1)
    {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        FD_SET(s, &set);

        int k= select(16, &set, NULL, NULL, &time);
        if (k == -1)
            printf("Select zwrocil blad: %s\n", strerror(errno));
        

        if (FD_ISSET(s, &set))
        {
            Message m;

            int l = recv(s, &m, sizeof(Message), 0);
            if (l == -1)
                printf ("Recv zwrocil blad: %s\n", strerror(errno));
            else if (l == 0)
            {
                printf("Serwer zamknal polaczenie\n");
                Interrupt(0);
            }

            //printf("Otrzymano wiadomosc od %d o typie %d z zawartoscia %s\n", m.senderID, m.messageType, m.content);
            if (m.messageType ==  MESSAGE_TYPE_PING)
            {
                //printf("\tWysylam ping\n");
                m.senderID=myID;
                strcat(m.content, "Response");

                if (send(s,&m,sizeof(Message),0) <= 0)
                    printf("\tCos poszlo nie tak!\n");
            }
            else if (m.messageType==MESSAGE_TYPE_RECEIVE_MESSAGE)
            {
                int id=-1;
                char content[1024];
                sscanf(m.content, "%d %1024[^\n]", &id, content);
                fflush(stdin);
                if (id == 0)
                    printf("Serwer: %s\n", content);
                else
                    printf("\n\tOtrzymano wiadomosc od %s[ID:%d]:%s\n\n", m.senderName, id, content);
            }

        }
        
        if (FD_ISSET(STDIN_FILENO, &set))
        {
            char input [MAX_CONTENT_LENGTH];

            //scanf("%s", input);

            fgets(input, MAX_CONTENT_LENGTH, stdin);

            fflush(stdin);
            if (strstr(input, "get") != NULL)
            {
                printf("Pobieram liste klientow...\n");
                Message req;
                req.senderID=myID;
                req.messageType = MESSAGE_TYPE_GET_CLIENTS;
                send(s, &req, sizeof(Message), 0);
                //there is a ping when retrieving clients list
                Message ping;
                recv(s,&ping, sizeof(Message),0);
                /*if (ping.messageType == MESSAGE_TYPE_PING)
                    printf("Otrzymano ping\n");*/

                ping.senderID=myID;
                strcat(ping.content, "Response");
                send(s,&ping,sizeof(Message), 0);

                Message m;
                recv(s, &m, sizeof(Message), 0);

                char* offset=m.content;
                printf("Podlaczeni uzytkownicy:\n");
                while (1)
                {
                    if (*(offset-1) == '.')
                        break;

                    char id[1024] = {0};
                    char name[1024] = {0};
                    char* firstComma=strchr(offset, ',');
                    //printf("first comma at:%ld\n", firstComma-offset);
                    strncpy(id, offset, firstComma-offset);
                    offset=firstComma+1;

                    char* secondComma=strchr(offset, ',');
                    if (secondComma == NULL)
                        secondComma=strchr(offset, '.');

                    //printf("second comma at:%ld\n", secondComma-offset);
                    strncpy(name, offset, secondComma-offset);
                    offset=secondComma+1;

                    printf("\t%s [ID:%d]\n", name, atoi(id));
                }

                PrintInfo();
            }
            else if (strstr(input, "send") != NULL)
            {
                int clientID;
                char message[1024];
                sscanf(input, "%*s %d %1024[^\n]", &clientID, message);

                if (clientID == 0)
                    {
                        printf("Nie mozna wyslac wiadomosci besposrednio do serwera.\n");
                        continue;
                    }
                
                Message req;
                req.senderID=myID;
                strcpy(req.senderName, username);
                sprintf(req.content, "%d %s", clientID, message);
                req.messageType=MESSAGE_TYPE_SEND_MESSAGE;
                send(s, &req, sizeof(req), 0);
                printf("Wyslano do %d wiadomosc: %s\n",clientID, message);
        
            }
            else if (strstr(input, "exit") != NULL)
            {
                Interrupt(0);
                break;
            }
            else
            {
                printf("Nierozpoznane polecenie\n");
            }
        }
    }
}