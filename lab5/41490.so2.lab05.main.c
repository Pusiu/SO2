// SO2 IS1 222B LAB05
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab05.main.c -o lab05
// ./lab05 -df /etc

#define OPTION_L 4
#define OPTION_d 2
#define OPTION_f 1
#define COLOR_NORMAL  "\x1B[0m"
#define COLOR_BOLDBLUE  "\033[1m\033[34m"
#define COLOR_BOLDGREEN  "\033[1m\033[32m"
#define COLOR_BOLDCYAN  "\033[1m\033[36m"


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>


typedef struct FolderStruct
{
    char folderPath[PATH_MAX];
    int indentLevel;
    DIR* dStream;
    int fileIndex;
    int fileCount;
    struct FolderStruct* parent;
} FolderStruct;

int directories = 0;
int files = 0;
FolderStruct** folderStack = NULL;
FolderStruct** allStructs = NULL;
int allStructsSize=0;
int folderStackSize = 0;
int indentAmount=1;

void ClearStack(FolderStruct*** stack, int* stackSize)
{
    if (*stack == NULL)
    {
        printf("Trying to free non allocated stack\n");
        return;
    }

    for (int i = 0; i < (*stackSize); i++)
    {
        free((*stack)[i]);
    }

    free((*stack));
}

void PushCopy(FolderStruct** s)
{
    allStructsSize++;
    allStructs = realloc(allStructs, allStructsSize*sizeof(FolderStruct*));
    allStructs[allStructsSize-1]=*s;
}

void Push(FolderStruct*** stack, int* stackSize, char *folderPath, int indentLevel, DIR* dStream, int fileIndex, int fileCount, FolderStruct* parent)
{
    (*stack) = realloc(*stack, sizeof(FolderStruct *) * ((*stackSize) + 1));
    for (int i = (*stackSize) - 1; i >= 0; i--)
    {
        (*stack)[i + 1] = (*stack)[i];
    }
    (*stack)[0] = malloc(sizeof(FolderStruct));
    strcpy((*stack)[0]->folderPath, folderPath);
    (*stack)[0]->indentLevel = indentLevel;
    (*stack)[0]->dStream=dStream;
    (*stack)[0]->fileCount=fileCount;
    (*stack)[0]->fileIndex=fileIndex;
    (*stack)[0]->parent=parent;
    PushCopy(&(*stack)[0]);
    (*stackSize)++;
}

FolderStruct* Pop(FolderStruct*** stack, int* stackSize)
{
    if (*stackSize == 0 || stack == NULL || *stack == NULL)
        return NULL;

    FolderStruct *out = (*stack)[0];

    (*stack)[0]=NULL;
    for (int i = 1; i < *stackSize; i++)
    {
        (*stack)[i - 1] = (*stack)[i];
    }
    (*stackSize)--;
    *stack = realloc(*stack, (*stackSize) * sizeof(FolderStruct*));

    return out;
}


bool ShouldPlacePipe(FolderStruct** parent, int index)
{
    FolderStruct* ptr = *parent;
    while (ptr != NULL)
    {
        if ((ptr->indentLevel == index) && (ptr->fileIndex < ptr->fileCount-1))
            return true;
        ptr=ptr->parent;
    }

    return false;
}

void PrintLine(char *name, int indentation, char* color, FolderStruct** parentFolder)
{
    for (int i = 0; i < indentation; i++)
    {
        printf((ShouldPlacePipe(parentFolder, i) ? "│   " : "    "));
    }
    if (*parentFolder != NULL)
        printf(((*parentFolder)->fileIndex >= (*parentFolder)->fileCount-1) ? "└──" : "├──");
    else
        printf("├──");

    printf("%s", color);
    printf(" %s%s", name, COLOR_NORMAL);
}



int main(int argc, char *argv[])
{
    char* startingDirectory = calloc(PATH_MAX, sizeof(char));

    int op;
    short mode = 0;
    int maxDepth=-1;
    //pobieranie opcji
    while ((op=getopt(argc, argv, "dfL:")) != -1)
    {
            switch (op)
            {
                    case 'd':
                            mode=mode | OPTION_d;
                            break;
                    case 'f':
                            mode = mode | OPTION_f;
                            break;
                    case 'L':
                            mode = mode | OPTION_L;
                            maxDepth=atoi(optarg);
                            break;
            }
    }

    getcwd(startingDirectory, PATH_MAX); //startingdirectory na poczatku jest sciezka do programu

    if (optind < argc)
    {
        if (argv[optind][0]=='/') //jesli nie relatywny to wklej argument jako starting directory 
        {
            strcpy(startingDirectory,argv[optind]);
        }
        else //w przeciwnym razie doklej do sciezki
        {
            strcat(startingDirectory, "/");
            strcat(startingDirectory,argv[optind]);
        }
        chdir(startingDirectory); //zmien working directory na starting directory
    }
    

    struct dirent** ent;

    Push(&folderStack, &folderStackSize, startingDirectory, 0, NULL, 0, 0, NULL); //wrzuc starting directory na stack
    FolderStruct* currentFolder;

    printf("%s%s%s\n", COLOR_BOLDBLUE,(optind < argc) ? argv[optind] : ".", COLOR_NORMAL); //print roota, kropka albo sciezka

    while (1)
    {
        chdir(startingDirectory); //zmien working dir na starting dir
        currentFolder = Pop(&folderStack, &folderStackSize); //zrzuc ze stosu folder 

        if (currentFolder == NULL) //jesli nie ma zadnego - koniec przeszukiwania
            break;

        DIR *ds = currentFolder->dStream; //directory stream
        if (ds == NULL) //jesli nie otworzony to otworz i ustaw go oraz index w strukturze na 0
        {
            ds=opendir(currentFolder->folderPath);
            currentFolder->dStream=ds;
            currentFolder->fileIndex=0;
        }

        if (ds == NULL) //jesli dalej nie utworzony to jest problem
        {
            printf("Nie mozna otworzyc katalogu: %s\n", currentFolder->folderPath);
            continue;
        }
        else
        {
            int currentDirCount = scandir(currentFolder->folderPath, &ent, NULL, &alphasort); //zlicz ilosc elementow w folderze, zrzuc nazwy do ent + posortuj alphasortem
            currentFolder->fileCount=currentDirCount; //ustawienie w strukturze ilosci plikow

            while (currentFolder->fileIndex < currentDirCount-1) //dopoki nie zczytane wszystkie pliki
            {
                (currentFolder->fileIndex)++; //zwieksz index
                
                bool print = true; //domyslnie nalezy wyswietlic element
                bool hasToBreak = false;


                struct dirent* entry = ent[currentFolder->fileIndex]; //info o elemencie

                if ((entry->d_name)[0]!='.') //pomija ukryte pliki oraz '.' i '..'
                {
                    char* color = COLOR_NORMAL;
                    char* fullPath = calloc(strlen(currentFolder->folderPath)+1+strlen(entry->d_name)+1, sizeof(char)); //calloc dla pelnej sciezki
                    strcat(fullPath, currentFolder->folderPath);
                    strcat(fullPath, "/");
                    strcat(fullPath, entry->d_name);

                    struct stat st;
                    lstat(fullPath, &st); //lstat zeby zgarnac informacje o linku, a nie pliku do ktorego wskazuje

                    switch (entry->d_type)
                    {
                    case DT_DIR:

                            if (currentFolder->fileIndex < currentDirCount-1) //jesli jeszcze sa pliki do odczytania to wrzuc obecny folder na stos zeby do niego wrocic
                            {
                            Push(&folderStack, &folderStackSize, currentFolder->folderPath, currentFolder->indentLevel, ds, currentFolder->fileIndex, currentDirCount, currentFolder->parent);
                            }
                            if (
                            ((mode & OPTION_L) && ((currentFolder->indentLevel+indentAmount)/indentAmount<maxDepth)) //jesli opcja L i glebokosc nastepnego folderu mniejsza/rowna maksymalnej
                            ||
                            ((mode & OPTION_L) == 0) //lub jesli nie ma opcji L
                            )
                            {
                                //to wrzuc odczytany folder na stos
                                Push(&folderStack, &folderStackSize, fullPath, currentFolder->indentLevel+indentAmount, NULL, 0, 0, currentFolder);
                            }
                            hasToBreak=true; //i wyjdz jak najszybciej z petli zeby go odczytac
                            directories++;
                            color = COLOR_BOLDBLUE;
                        break;
                    case DT_REG:
                        if (mode & OPTION_d)
                            print=false; //nie printuj jesli opcja d wlaczona
                        else
                        {
                            if (st.st_mode & S_IXUSR)
                                color=COLOR_BOLDGREEN;

                            files++;
                        }
                        break;
                    }

                    if (print)
                    {                                    
                        int rootLength=strlen(startingDirectory);
                        char* toAdd = (optind < argc) ? argv[optind] :  ".";

                        char* path = (mode & OPTION_f) ? calloc(strlen(currentFolder->folderPath)-rootLength+strlen(entry->d_name)+2+strlen(toAdd), sizeof(char)) 
                                                        : calloc(strlen(entry->d_name)+1, sizeof(char));

                        if (mode & OPTION_f) //jesli pelne sciezki to wpisz odpowiednio
                        {
                            strcat(path,toAdd);
                            strcat(path, &(currentFolder->folderPath[rootLength]));
                            strcat(path,"/");
                        }

                        strcat(path, entry->d_name);

                        if (S_ISLNK(st.st_mode)) //jesli to link, to wymaga specjalnego traktowania
                        {
                                chdir(currentFolder->folderPath);
                                char* f = calloc(st.st_size + 1, sizeof(char));
                                readlink(fullPath, f, st.st_size+1);
                                f[st.st_size]='\0';

                                struct stat pointed;
                                bool exists = (stat(f,&pointed) == 0);
                                bool isDir = false;
                                if (exists)
                                    isDir = S_ISDIR(pointed.st_mode);

                            if (
                                (((mode & OPTION_d) != 0) && isDir)
                                ||
                                ((mode & OPTION_d) == 0)
                                ) //printuj tylko jesli opcja d (czyli 2) i jest folderem lub jesli nie ma opcji d (czyli po & jest 0)
                            {
                                PrintLine("", currentFolder->indentLevel, color, &currentFolder);
                                if (exists)
                                    printf("%s", COLOR_BOLDCYAN);
                                else
                                    printf("\033[31;40;1m");

                                if (isDir)
                                {
                                    color = COLOR_BOLDBLUE;
                                    directories++;
                                }
                                else
                                    files++;

                                printf("%s", path);
                                printf("%s", COLOR_NORMAL);
                                printf(" -> %s%s%s",  color, f, COLOR_NORMAL);
                                printf("\n");
                            }
                                free(f);
                        }
                        else
                        {
                            PrintLine(path, currentFolder->indentLevel, color, &currentFolder);
                            printf("\n");
                        }

                        free(path);
                    
                    }
                    free(fullPath);

                    if (currentFolder->fileIndex >= currentFolder->fileCount-1)
                    {
                        closedir(ds);
                        currentFolder->dStream=NULL;
                    }

                    if (hasToBreak)
                        break;
                }
            }

            for (int i=0; i < currentDirCount; i++)
                free(ent[i]);
            free(ent);
        }
    }

    ClearStack(&allStructs, &allStructsSize);

    printf("\n%d katalogow",directories);
    if ((mode & OPTION_d) == 0)
        printf(", %d plikow",files);
    
    printf("\n");
    free(startingDirectory);
}