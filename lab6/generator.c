#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>

int main(int argc, char* argv[])
{
    char* input = argv[1];
    char* salt = "5MfvmFOaDU";
    char* cryptMethod="6";

    char* fullSalt = calloc(strlen(input)+strlen(salt)+strlen(cryptMethod)+4, sizeof(char));
    strcat(fullSalt, "$");
    strcat(fullSalt, cryptMethod);
    strcat(fullSalt, "$");
    strcat(fullSalt, salt);
    strcat(fullSalt, "$");
    
    struct crypt_data data;
    data.initialized=0;

    char* output = crypt_r(input, fullSalt, &data);
    printf("'%s'\n", output);
}