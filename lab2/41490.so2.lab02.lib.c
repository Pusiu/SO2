#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <memory.h>

char *GetGroups(uid_t userID)
{
        char *output = malloc(2);
        strcpy(output, "[");

        struct passwd *pw = NULL; //do wyciagniecia informacji o userze za pomoca uid
        struct group *gr = NULL; //do przechowywania informacji o obecnie sprawdzanej grupie

        int maxGroupLength = 1;
        gid_t *groups = malloc(sizeof(gid_t));

        pw = getpwuid(userID); //zebranie info o userze za pomoca uid


        getgrouplist(pw->pw_name, pw->pw_gid, groups, &maxGroupLength); //zebranie liczby grup, po tym wywolaniu liczba grup znajduje sie w maxGroupLength
        groups = realloc(groups, maxGroupLength * sizeof(gid_t)); //realloc zeby bylo miejsce na wszystkie grupy
        getgrouplist(pw->pw_name, pw->pw_gid, groups, &maxGroupLength); //wlasciwe zebranie gid_t do groups

        for (int i = 0; i < maxGroupLength; i++)
        {
                gr = getgrgid(groups[i]); //zebranie info o grupie
                if (gr != NULL)
                {
                        if (i == 0)
                        {
                                output = realloc(output, strlen(output) + strlen(gr->gr_name) + 1);
                                strcat(output, gr->gr_name);
                        }
                        else
                        {
                                output = realloc(output, strlen(output) + strlen(gr->gr_name) + strlen(", ") + 1);
                                strcat(output, ", ");
                                strcat(output, gr->gr_name);
                        }
                }
        }
        free(groups);

        output = realloc(output, strlen(output) + strlen("]") + 1);
        strcat(output, "]");
        return output;
}