// SO2 IS1 222B LAB04
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab04.main.c -o lab04 -lm
// ./lab04 abcde

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <signal.h>

bool interrupted = false;
bool isParent = false;
bool isMaster = false;

bool IsPowerOfTwo(int x)
{
      return (ceil(log2(x)) == floor(log2(x))); 
}

int GetNearestPowerOfTwo(int x)
{
    while (!IsPowerOfTwo(x))
    {
        x++;
    }

    return x;
}

void Interrupt(int s)
{
    interrupted=true;
    if (isMaster)
    printf("\n");
}


int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("Nalezy przekazac conajmniej jeden parametr\n");
        exit(1);
    }
    char* param = malloc(strlen(argv[argc-1])+1); //+1 bo terminating 0
    strcpy(param, argv[argc-1]);

    setpgid(0,0); //ustawia grupe procesu na pid obecnego procesu
    pid_t firstChild = -1;
    pid_t secondChild = -1;

    sigset_t iset;
    struct sigaction act;

    sigemptyset(&iset);
    act.sa_handler = &Interrupt;
    act.sa_mask=iset;
    act.sa_flags=0;

    if (argc == 2) //jesli macierzysty
    {
        isMaster=true;
        sigaddset(&iset, SIGTSTP);
        sigprocmask(SIG_BLOCK, &iset, NULL);
    }

    sigaction(SIGINT, &act, NULL);

    if (strlen(param) > 1) //jesli dlugosc argumentu jest wieksza niz 1, to jest rodzicem
        isParent=true;

    if (isParent)
    {
        int len = strlen(param);
        if (!IsPowerOfTwo(len)) //jesli dlugosc nie jest potega 2
        {
            int x = GetNearestPowerOfTwo(len); //najblizsza potega w gore
            char nextChar=param[len-1]+1; //ostatni znak+1
            param = realloc(param, x+1); //rozszerzenie pamieci
            for (int i=0; i < x-len;i++)
            {
                param[len+i]=(char)(nextChar+i); //wypelnienie kolejnymi znakami
            }
            param[x+1]=0; //terminating 0
            printf("Argument musial zostac wydluzony, %s (%d) -> %s (%d)\n", argv[argc-1], len, param, x);
            len=x; //teraz dlugosc jest x (bez 0)
        }
        char* firstHalf = malloc(len/2 + 1);
        char* secondHalf = malloc(len/2 + 1);
        for (int i=0; i < len; i++)
        {
            if (i < len/2)
                firstHalf[i]=param[i];
            else
                secondHalf[i-len/2]=param[i];
        }
        firstHalf[len/2]=0;
        secondHalf[len/2]=0;
        

        char* programPath = argv[0];
        char* arguments[argc+2];
        arguments[0] = argv[0]; //pierwszy to nazwa programu
        for (int i=1; i<argc; i++)
        {
            arguments[i]=argv[i]; //poprzednie argumenty
        }
        arguments[argc+1] = NULL; //ostatni to null terminator
        

        firstChild = fork();
        if (firstChild == -1)
	    {
            printf("[%d] Nie udalo sie utworzyc potomnego procesu nr 1", getpid());
		    exit(1);
	    }

        if (firstChild != 0) //fork dla drugiego procesu nastapi tylko w rodzicu
        { 
            secondChild = fork();
            if (secondChild == -1)
            {
                printf("[%d] Nie udalo sie utworzyc potomnego procesu nr 2", getpid());
                exit(1);
            }

            if (secondChild == 0) //jesli to drugi proces
            {
                arguments[argc] = secondHalf; //drugi to pierwszy argument
                if (execv(programPath, arguments) == -1)
                {
                   printf("\n[%d] wystapil problem z execv:\n%d\n", getpid(), errno);
                   exit(1);
                }
            }
        }
        else //jesli to pierwszy proces
        {
            arguments[argc] = firstHalf; //drugi to pierwszy argument
            if (execv(programPath, arguments) == -1)
            {
                printf("\n[%d] wystapil problem z execv\n%d\n", getpid(), errno);
                exit(1);
            }
        }
        free(firstHalf);
        free(secondHalf);
    }

    while (1)
    {
        if (interrupted)
        {
            if (isParent)
            {
                kill(firstChild, SIGINT);
                kill(secondChild, SIGINT);
            }
            break;
        }
    }

    if (isParent)
    {
        int status;
        wait(&status);
        if (!WIFEXITED(status))
        {
            printf("\n[%d]Problem z wait #1: %d", getpid(), status);
            exit(1);
        }
        wait(&status);
        if (!WIFEXITED(status))
        {
            printf("\n[%d]Problem z wait #2: %d", getpid(), status);
            exit(1);
        }
    }
    free(param);

    pid_t thisPID = getpid();

    printf("%d", thisPID);
    for (int i=1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    if (isMaster) //proces macierzysty
    {
        printf("\n");
        sigset_t set; //struktura trzymajaca sygnaly
        sigpending(&set); //zbierz sygnaly wyslane w trakcie dzialania
        if (sigismember(&set, SIGTSTP)) //jesli byl sygnal sigtstp
        {
            printf("Podczas trwania programu zostal wyslany sygnal SIGTSTP\n");
        }
        sigprocmask(SIG_UNBLOCK, &iset, NULL); //odblokowanie obslugi sigtstp, przy ctrl+z na koniec master przenosi sie do tla
    }

    return 0;
}
