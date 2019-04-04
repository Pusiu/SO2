// SO2 IS1 222B LAB02
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab02.main.c -o lab2 -ldl; gcc -c -fPIC 41490.so2.lab02.lib.c; gcc -shared -fPIC 41490.so2.lab02.lib.o -o 41490.so2.lab02.libgroups.so.0
// ./lab2 -hg


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <memory.h>
#include <pwd.h>
#include <dlfcn.h>

char* (*GetGroups) (uid_t userID); //wskaznik do funkcji z biblioteki

int main(int argc, char* argv[])
{
        int op = 0;
        short mode = 0;
        while ((op=getopt(argc, argv, "hg")) != -1)
        {
                switch (op)
                {
                        case 'h':
                                mode=mode | 1;
                                break;
                        case 'g':
                                mode = mode | 2;
                                break;
                }
        }
        printf("\n");
        struct utmp * ut = NULL;
        void *handle = NULL; //handle na biblioteke
        if (mode & 2)
        {
                handle = dlopen("./41490.so2.lab02.libgroups.so.0", RTLD_LAZY); //otworzenie bilbioteki
                //RTLD_LAZY=Perform lazy binding. Only resolve symbols as the code that references them is executed
                if (!handle)
                {
                        printf ("%s\n", dlerror());
                }
                
                GetGroups = dlsym(handle, "GetGroups"); //przypisanie wlasciwej funkcji do wskaznika na nia
        }

        setutent();
        while((ut = getutent()) != NULL)
        {
                if (ut->ut_type == USER_PROCESS)
                {
                        printf("%s", ut->ut_name);
                        if (handle) //gdy nie ma biblioteki - dziala jak bez przelacznikow
                        {
                                if (mode & 1)
                                        printf(" (%s) ", ut->ut_host);
                                if (mode & 2)
                                {
                                        struct passwd* pw = getpwnam(ut->ut_name);

                                        char* out = GetGroups(pw->pw_uid);
                                        printf(" %s", out); //wywolanie funkcji z biblioteki
                                        free(out);
                                }
                        }
                        printf("\n");
                }
        }
        endutent();

        if (handle)
        {
                dlclose(handle); //zamkniecie biblioteki 
        }

        printf("\n");
        return 0;
}
