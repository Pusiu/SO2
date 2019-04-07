// SO2 IS1 222B LAB06
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab06.main.c -o lab06 -lpthread -lcrypt  
//  ./lab06 $6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1 ./small 4

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>


bool foundSolution=false;
char* passwordsFile="small.txt";
char* offset = NULL;
unsigned int filesize = 0;
char* mappedFile = NULL;
char* fullHash = NULL;
char cryptMethod = '6';
char* salt = NULL;
char* hash = NULL;

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


char* ReadLine()
{
    char* newline=memchr(offset, '\n', 1024);
    if (newline == NULL)
    {
        printf("readline null\n");
        return NULL;
    }
    int length = (unsigned int)(newline-offset);
    printf("Character: %d", newline);
    printf(" at %d", length);

    char* output = malloc(length+2);
    memcpy(output, offset, length);
    //output[length]='\0';
    offset=newline+1;
    printf("Returning: %s", output);
    return output;
}

void Crack()
{
    struct crypt_data data;
    data.initialized = 0;
    while (!foundSolution)
    {
        char* line = ReadLine();
        if (line == NULL)
            {
                free(line);
                break;
            }
        printf("Read line: %s", line);
        char* hash = crypt_r(line, salt, &data);
        printf("Trying to crack: %s\nHash:%s", line, hash);
        
        if (hash == fullHash)
        {
            foundSolution=true;
            printf("Solution found!\nOriginal password: %s", line);
        }

        free(line);
    }
}

int main(int argc, char* argv[])
{
    int numOfThreads =  sysconf(_SC_NPROCESSORS_ONLN);



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

    struct stat st; //stat for getting file size
    stat(passwordsFile,&st);
    int fd = open(passwordsFile, O_RDONLY,0);
    mappedFile = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    offset=mappedFile;
    
    Crack();

    //char* crypted = crypt_r("aarek1940", "$6$5MfvmFOaDU$", &data);
    //printf("%d\n%c\n%s\n%s\n%s\n",numOfThreads,cryptMethod, salt, hash, crypted);
    if (argc >= 2)
    {
        free(fullHash);
        free(salt);
        free(hash);
    }
    munmap(mappedFile, st.st_size);
    close(fd);
}