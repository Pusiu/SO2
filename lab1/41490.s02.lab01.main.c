// SO2 IS1 222B LAB01
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab01.main.c -o lab1
// ./lab1 -hg


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <pwd.h>
#include <grp.h>
#include <memory.h>

int main(int argc, char* argv[])
{
        int op;
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
        struct utmp * ut;
        struct passwd *pw;
        struct group *gr;
        int maxGroupLength=getgroups(0, NULL);
        gid_t *groups = malloc(maxGroupLength * sizeof(gid_t));

        setutent();
        while((ut = getutent()) != NULL)
        {
                if (ut->ut_type == 7)
                {
                        printf("%s", ut->ut_name);
                        if (mode & 1)
                                printf(" (%s) ", ut->ut_host);
                        if (mode & 2)
                        {
                                pw = getpwnam(ut->ut_name);
                                getgrouplist(ut->ut_name, pw->pw_gid, groups, &maxGroupLength);
                                printf("[");
                                for (int i=0; i < maxGroupLength; i++)
                                {
                                        gr = getgrgid(groups[i]);
                                        if (gr != NULL)
                                        {
                                                if (i == 0)
                                                        printf ("%s", gr->gr_name);
                                                else
                                                        printf (", %s", gr->gr_name);
                                        }
                                }
                                printf("]");
                        }
                        printf("\n");
                }
        }
        endutent();


        printf("\n");
        return 0;
}
