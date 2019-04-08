// SO2 IS1 222B LAB06
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab06.main.c -o lab06 -lpthread -lcrypt  
// ./lab06 \$6\$5MfvmFOaDU\$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1 ./small 4

#define _GNU_SOURCE
#define SOLUTION_IN_PROGRESS 0
#define SOLUTION_FOUND 1
#define SOLUTION_NOT_FOUND 2

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
#include <pthread.h>
#include <errno.h>

short runningThreadsMask =0;
char solutionStatus = SOLUTION_IN_PROGRESS;
char* passwordsFile="small.txt";
char* offset = NULL;
unsigned int filesize = 0;
char* mappedFile = NULL;
char* fullHash = NULL;
char cryptMethod = '6';
char* salt = NULL;
char* saltAndCryptMethod=NULL;
char* hash = NULL;
unsigned int totalPasswords = 0;
unsigned int passwordProgress =0;
pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runningThreadsMaskMutex = PTHREAD_MUTEX_INITIALIZER;

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

int CountLines()
{
    int o=0;
    char* nextAddr=mappedFile;
    while (1)
    {
        char* newLine=memchr(nextAddr, '\n', 1024);
        if (newLine == NULL)
            return o;
        o++;
        nextAddr=newLine+1;
    }
}

char* ReadLine()
{
    char* newline=memchr(offset, '\n', 1024);
    if (newline == NULL)
    {
        return NULL;
    }
    int length = (unsigned int)(newline-offset);

    char* output = malloc(length+2);
    memcpy(output, offset, length);
    output[length]='\0';
    offset=newline+1;
   // printf("Returning: %s", output);
    return output;
}

void UpdateProgress()
{
    if (solutionStatus == SOLUTION_IN_PROGRESS)
    {
        printf("\33[2K\rCracking progress: %.2f%%",((float)passwordProgress/totalPasswords)*100);
        //printf("\033[5D%.2f%%",((float)passwordProgress/totalPasswords)*100);
    }
}

void* Crack(void* args)
{
    struct crypt_data data;
    data.initialized = 0;
    int threadID=*((int*)args)-1;

    while (solutionStatus == SOLUTION_IN_PROGRESS)
    {
        char* line = ReadLine();
        if (line == NULL)
            {
                free(line);
                break;
            }
        char* hash = crypt_r(line, saltAndCryptMethod, &data);
       // printf("Trying to crack: %s\nHash:%s\n", line, hash);

        pthread_mutex_lock(&progressMutex);
        passwordProgress++;
        //UpdateProgress();
        pthread_mutex_unlock(&progressMutex);
        
        if (strcmp(fullHash,hash)==0)
        {
            solutionStatus = SOLUTION_FOUND;
            printf("\n[%d]Solution found!\n%s\nis the same as\n%s\nOriginal password: %s\n",threadID, fullHash, hash, line);
        }
        free(line);
    }
    pthread_mutex_lock(&runningThreadsMaskMutex);
    runningThreadsMask = runningThreadsMask - runningThreadsMask & (1 << threadID);
    pthread_mutex_unlock(&runningThreadsMaskMutex);
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
        saltAndCryptMethod=malloc(strlen(salt)+3);
        strncpy(saltAndCryptMethod, fullHash, strlen(salt)+3);
    }

    struct stat st; //stat for getting file size
    stat(passwordsFile,&st);
    int fd = open(passwordsFile, O_RDONLY,0);
    mappedFile = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    offset=mappedFile;
    totalPasswords = CountLines();

    
    pthread_t threadIDs[numOfThreads];
    printf("Creating %d threads.\n", numOfThreads);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i=0; i < numOfThreads; i++)
    {
        errno=pthread_create(&threadIDs[i],NULL,Crack,&i);
        runningThreadsMask=runningThreadsMask | (1 << i);
    }
    pthread_attr_destroy(&attr);

    printf("Cracking progress: 0.00%%");
    while (solutionStatus == SOLUTION_IN_PROGRESS && (runningThreadsMask != 0) && (passwordProgress < totalPasswords))
    {
        UpdateProgress();
        usleep(5000); //sleep 0.5s
    }
    printf("Mask:%d\n", runningThreadsMask);

    for (int i=0; i < numOfThreads; i++) {
		errno = pthread_join(threadIDs[i], NULL);
	}
    pthread_mutex_destroy(&progressMutex);
    pthread_mutex_destroy(&runningThreadsMaskMutex);


    //char* crypted = crypt_r("aarek1940", "$6$5MfvmFOaDU$", &data);
    //printf("%d\n%c\n%s\n%s\n%s\n",numOfThreads,cryptMethod, salt, hash, crypted);
    if (argc >= 2)
    {
        free(fullHash);
        free(salt);
        free(hash);
        free(saltAndCryptMethod);
    }
    munmap(mappedFile, st.st_size);
    close(fd);
}