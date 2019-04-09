// SO2 IS1 222B LAB06
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab06.main.c -o lab06 -lpthread -lcrypt  
// ./lab06 '$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1' ./small 4



#define _GNU_SOURCE
#define MAX_TEST_LINES 1000
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
#include <time.h>

bool threadTesting = false;
int numOfThreads = 1;
short runningThreadsMask =0;
char solutionStatus = SOLUTION_IN_PROGRESS;
char* passwordsFile="./small.txt";
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
double totalTime;
pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runningThreadsMaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t totalTimeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readLineMutex = PTHREAD_MUTEX_INITIALIZER;

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
    return output;
}

void UpdateProgress()
{
    if (solutionStatus == SOLUTION_IN_PROGRESS)
    {
        //printf("\33[2K\rCracking progress: %.2f%%",((float)passwordProgress/totalPasswords)*100);
        float pr = ((float)passwordProgress/totalPasswords)*100;

        printf("\033[%dD%.2f%%",(pr > 10) ? 6 : 5, pr);
    }
}

void* Crack(void* args)
{

    int threadID=*((int*)args);

    while (solutionStatus == SOLUTION_IN_PROGRESS)
    {
        struct crypt_data data;
        data.initialized = 0;

        if (threadTesting && passwordProgress >= MAX_TEST_LINES)
            break;

        pthread_mutex_lock(&readLineMutex);
        char* line = ReadLine();
        pthread_mutex_unlock(&readLineMutex);
        if (line == NULL)
        {
            free(line);
            break;
        }

        pthread_mutex_lock(&progressMutex);
        passwordProgress++;
        pthread_mutex_unlock(&progressMutex);
        char* hash = crypt_r(line, saltAndCryptMethod, &data);
        if (hash == NULL)
            printf("crypt returned null!\n");

        if (strcmp(fullHash,hash)==0 && !threadTesting)
        {
            solutionStatus = SOLUTION_FOUND;
            printf("\n[%d]Znaleziono rozwiazanie.\n%s\nodpowiada\n%s\nOryginalne haslo: %s\n",threadID, fullHash, hash, line);
        }
        free(line);
    }
    pthread_mutex_lock(&runningThreadsMaskMutex);
    runningThreadsMask = runningThreadsMask - runningThreadsMask & (1 << threadID);
    pthread_mutex_unlock(&runningThreadsMaskMutex);

    if (threadTesting)
    {
        clockid_t clock;
        pthread_getcpuclockid(pthread_self(), &clock);
        struct timespec ts;
        clock_gettime(clock, &ts);
        printf("\tWatek %d dzialal przez %ld.%03ld s\n", threadID, ts.tv_sec, ts.tv_nsec / 1000000);
        pthread_mutex_lock(&totalTimeMutex);
        totalTime+=ts.tv_sec;
        totalTime+=ts.tv_nsec / 1000000000.0;
        pthread_mutex_unlock(&totalTimeMutex);
    }
}


void TestThreads()
{
    int curThreadCount = 1;
    printf("Rozpoczeto testowanie watkow.\nPlik haslowy:%s\nMaksymalna ilosc linii do odczytania:%d\n\n", passwordsFile, MAX_TEST_LINES);
    while (curThreadCount <= numOfThreads)
    {
        runningThreadsMask = 0;
        offset=mappedFile;
        totalTime=0;
        printf("Lamanie z uzyciem %d watkow:\n", curThreadCount);
        passwordProgress = 0;
        pthread_t threads[curThreadCount];

        for (int i=0; i < curThreadCount; i++)
        {
            int* id=malloc(sizeof(int));
            *id=i;
            pthread_create(&threads[i], NULL, Crack, id);
            runningThreadsMask=runningThreadsMask | (1 << i);
        }
        for (int i=0; i < curThreadCount; i++)
        {
            pthread_join(threads[i], NULL);
        }
        printf("Proces lamania trwal %f sekund\n", totalTime/(curThreadCount));
        curThreadCount++;
    }
}

int main(int argc, char* argv[])
{
    numOfThreads =  sysconf(_SC_NPROCESSORS_ONLN);

    if (argc >= 2)
    {
        fullHash = malloc(sizeof(char) * strlen(argv[1])+1);
        strcpy(fullHash, argv[1]);
        cryptMethod=fullHash[1];
        int a = GetCharIndex(fullHash, '$', 3);
        salt=calloc(a-2+1, sizeof(char));
        hash=calloc(strlen(fullHash)-a+1,sizeof(char));
        strncpy(salt, &fullHash[3], a-2);
        strcpy(hash, &fullHash[a+1]);
        saltAndCryptMethod=calloc(strlen(salt)+4, sizeof(char));
        strncpy(saltAndCryptMethod, fullHash, strlen(salt)+3);
        passwordsFile = argv[2];
        if (argc == 4)
        {
            if (atoi(argv[3]) < numOfThreads)
                numOfThreads = atoi(argv[3]);
        }
        else
            threadTesting=true;
    }

    struct stat st; //stat for getting file size
    stat(passwordsFile,&st);
    int fd = open(passwordsFile, O_RDONLY,0);
    mappedFile = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    if (mappedFile == MAP_FAILED)
    {
        printf("Nie mozna zaladowac pliku do pamieci\n");
        exit(EXIT_FAILURE);
    }

    offset=mappedFile;
    totalPasswords = CountLines();

    if (!threadTesting)
    {
        pthread_t threadIDs[numOfThreads];
        printf("Informacje:\nHash: %s\nNazwa pliku z haslami: %s\nIlosc hasel:%d\nIlosc watkow:%d\n",fullHash, passwordsFile, totalPasswords, numOfThreads);
        for (int i=0; i < numOfThreads; i++)
        {
            errno=pthread_create(&threadIDs[i],NULL,Crack,&i);
            runningThreadsMask=runningThreadsMask | (1 << i);
        }

        printf("Postep lamania:        ");
        while (solutionStatus == SOLUTION_IN_PROGRESS && (runningThreadsMask != 0) && (passwordProgress < totalPasswords))
        {
            UpdateProgress();
            usleep(5000); //sleep 0.5s
        }
        UpdateProgress();
        //printf("\nProgress:%d/%d\nMask:%d\n",passwordProgress, totalPasswords, runningThreadsMask);

        for (int i=0; i < numOfThreads; i++) {
            errno = pthread_join(threadIDs[i], NULL);
        }
        printf("\n");
    }
    else
        TestThreads();

    
    pthread_mutex_destroy(&progressMutex);
    pthread_mutex_destroy(&runningThreadsMaskMutex);
    pthread_mutex_destroy(&totalTimeMutex);
    pthread_mutex_destroy(&readLineMutex);

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