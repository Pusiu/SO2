// SO2 IS1 222B LAB06
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc -lpthread -lcrypt 41490.so2.lab06.main.c -o lab06 
//  ./lab06 $6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1 ./small 4

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>


int GetCharIndex(char* const source, char character, int occurance)
{
    int occurances=0;
    int len = strlen(source);
    for (int i=0; i < len;i++)
    {
        if (source[i]==character )
        {
            occurances++;
            if (occurances==occurance)
                return i;
        }
    }
    return -1;
}

int main(int argc, char* argv[])
{
    int numOfThreads =  sysconf(_SC_NPROCESSORS_ONLN);

    char* fullHash = NULL;
    char cryptMethod = '6';
    char* salt = NULL;
    char* hash = NULL;

    if (argc >= 2)
    {
        fullHash = malloc(sizeof(char) * strlen(argv[1])+1);
        strcpy(fullHash, argv[1]);
        cryptMethod=fullHash[1];
        int a = GetCharIndex(fullHash, '$', 3);
        salt=malloc(sizeof(char) * a-2+1);
        hash=malloc(sizeof(char) * strlen(fullHash)-a+1);
        strncpy(salt, &fullHash[3], a-3);
        strcpy(hash, &fullHash[a+1]);
    }

    struct crypt_data data;
    data.initialized = 0;
    
    char* crypted = crypt_r("aarek1940", "$6$5MfvmFOaDU$", &data);
    printf("%d\n%c\n%s\n%s\n%s\n",numOfThreads,cryptMethod, salt, hash, crypted);
    if (argc >= 2)
    {
        free(fullHash);
        free(salt);
        free(hash);
    }
}