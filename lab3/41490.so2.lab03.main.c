// SO2 IS1 222B LAB03
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab03.main.c -o lab03 -lm
// ./lab03 abcde

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

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

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("Nalezy przekazac conajmniej jeden parametr\n");
        exit(1);
    }
    char* param = malloc(strlen(argv[argc-1])+1); //+1 bo terminating 0
    strcpy(param, argv[argc-1]);


    if (strlen(param) > 1)
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
        

        pid_t firstChild = fork();
        if (firstChild == -1)
            printf("Nie udalo sie utworzyc potomnego procesu nr 1");

        if (firstChild != 0) //fork dla drugiego procesu nastapi tylko w rodzicu
        { 
            pid_t secondChild = fork();
            if (secondChild == -1)
                printf("Nie udalo sie utworzyc potomnego procesu nr 2");

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
        if (wait(NULL) == -1)
            printf("[%d]Problem z wait #1", getpid());
        if (wait(NULL) == -1)
            printf("[%d]Problem z wait #2", getpid());
        free(firstHalf);
        free(secondHalf);
    }
    
    pid_t thisPID = getpid();

    printf("%d ", thisPID);
    for (int i=1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");

    free(param);
    return 0;
}