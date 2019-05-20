// SO2 IS1 222B LAB10
// Gracjan Puch
// pg41490@zut.edu.pl
// gcc 41490.so2.lab10.main.c -o lab10
// ./lab10 disk-image

#define _GNU_SOURCE

#define BLOCK_SIZE 1024
#define COLOR_NORMAL  "\33[00m"
#define COLOR_DIRECTORY  "\033[01;34m"
#define COLOR_WRITABLE_DIRECTORY  "\033[30;42m"
#define COLOR_EXECUTABLE  "\033[01;32m"
#define COLOR_FIFO  "\033[40;33m"
#define COLOR_SOCKET  "\033[01;35m"
#define COLOR_BLK  "\033[40;33m"
#define COLOR_CHR  "\033[40;33m"
#define COLOR_LINK  "\033[01;36m"

#include "so2ext2.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <time.h>

int fd = -1;
int groupCount = -1;
unsigned int inodesPerGroup;
unsigned int inodesTotal;
struct ext2_group_desc** groups;
int totalDirectories;
int totalFiles;
int currentIndent=1;

unsigned int BlockToOffset(unsigned int block)
{
    return BLOCK_SIZE+(block-1)*BLOCK_SIZE;
}


void GetInode(int fd, unsigned int inode, struct ext2_inode* inodeStruct)
{
    int ind=0;
    unsigned int groupNumber = inode/inodesPerGroup;
    unsigned int index=inode % inodesPerGroup;
    do {
        int offset=BlockToOffset(groups[groupNumber]->bg_inode_table)+(index-1)*sizeof(struct ext2_inode);
        lseek(fd, offset, SEEK_SET);
        read(fd, inodeStruct, sizeof(struct ext2_inode));
        ind++;
        if (inodeStruct->i_size != 0)
            return;
    }while (ind < groupCount);
}

void PrettyPrint(struct ext2_inode* inode, int type, char* filename, unsigned int inodeNumber)
{
    printf("%s", COLOR_NORMAL);
    printf("├");
    for (int i=0; i < currentIndent; i++)
        printf("──");
    printf(" [");

    if (type == 2)
        printf("%s", "d");
    else if (type == 1)
        printf("%s", "-");
    else if (type == 3)
        printf("%s", "c");
    else if (type == 4)
        printf("%s", "b");
    else if (type == 5)
        printf("%s", "p");
    else if (type == 7)
        printf("%s", "l");
    else if (type == 6)
        printf("%s", "s");


    printf("%c", (S_IRUSR & inode->i_mode) ? 'r' : '-');
    printf("%c", (S_IWUSR & inode->i_mode) ? 'w' : '-');
    printf("%c", (S_IXUSR & inode->i_mode) ? 'x' : '-');
    printf("%c", (S_IRGRP & inode->i_mode) ? 'r' : '-');
    printf("%c", (S_IWGRP & inode->i_mode) ? 'w' : '-');
    printf("%c", (S_IXGRP & inode->i_mode) ? 'x' : '-');
    printf("%c", (S_IROTH & inode->i_mode) ? 'r' : '-');
    printf("%c", (S_IWOTH & inode->i_mode) ? 'w' : '-');
    printf("%c", (S_IXOTH & inode->i_mode) ? 'x' : '-');


    struct passwd *pw;
    pw=getpwuid(inode->i_uid);
    printf(" %s", pw->pw_name);
    printf("\tinode: %d\t", inodeNumber);
    printf("%d ", inode->i_size / 1024);
    time_t t = (time_t)inode->i_ctime;
    printf("%.24s", ctime(&t));
    printf("] ");

    if (type == 2)
        if ((S_IWUSR & inode->i_mode) || (S_IWGRP & inode->i_mode) || (S_IWOTH & inode->i_mode))
            printf("%s", COLOR_WRITABLE_DIRECTORY);
        else
            printf("%s", COLOR_DIRECTORY);
    else if (type == 1)
        if ((S_IXUSR & inode->i_mode) || (S_IXGRP & inode->i_mode) || (S_IXOTH & inode->i_mode))
            printf("%s", COLOR_EXECUTABLE);
        else
            printf("%s", COLOR_NORMAL);
    else if (type == 3)
        printf("%s", COLOR_CHR);
    else if (type == 4)
        printf("%s", COLOR_BLK);
    else if (type == 5)
        printf("%s", COLOR_FIFO);
    else if (type == 7)
        printf("%s", COLOR_LINK);
    else if (type == 6)
        printf("%s", COLOR_SOCKET);

    printf("%s", filename);
    

    printf("\n");
}

void ReadDirectory(struct ext2_inode* inode)
{
        unsigned int offset=0;
        while (offset < inode->i_size)
        {
            lseek(fd, BlockToOffset(inode->i_block[0])+offset, SEEK_SET);
            struct ext2_dir_entry dir;
            read(fd, &dir, sizeof(struct ext2_dir_entry));

            char* filename = calloc(dir.name_len,sizeof(char));
            strncpy(filename, dir.name, dir.name_len);

            struct ext2_inode* info = calloc(1, sizeof(struct ext2_inode));
            GetInode(fd, dir.inode, info);

            offset+=dir.rec_len;

            if (*filename != '.')
            {
                PrettyPrint(info,dir.file_type, filename, dir.inode);
                if (dir.file_type == 2)
                {
                    if (strcmp(filename, "lost+found") != 0)
                    {
                        currentIndent++;
                        ReadDirectory(info);
                        currentIndent--;
                    }
                    totalDirectories++;
                }
                else
                    totalFiles++;
            }

            free(info);

            free(filename);
        }
}

void main(int argc, char* argv[])
{
    if (argc == 1 || argc > 2)
    {
        printf ("Program nalezy uruchomic z jednym argumentem - nazwa obrazu dyskowego.\n");
        exit(EXIT_FAILURE);
    }

    char* filename=argv[1];
    fd = open(filename, O_RDONLY, 0);
    if (fd == -1)
    {
        printf("Wystapil blad przy otwieraniu pliku: %s\n", strerror(errno));
    }

    char buf[BLOCK_SIZE];

    read(fd, buf, BLOCK_SIZE); //read boot block

    struct ext2_super_block superblock;
    read(fd, &superblock, sizeof(struct ext2_super_block)); //read superblock
    groupCount=(superblock.s_blocks_count/superblock.s_blocks_per_group)+1;
    inodesPerGroup=superblock.s_inodes_per_group;

    groups = calloc(2, sizeof(struct ext2_group_desc));
    for (int i=0; i < groupCount; i++)
    {
        lseek(fd, BLOCK_SIZE+sizeof(struct ext2_super_block)+(i*sizeof(struct ext2_group_desc)), SEEK_SET); //seek behind boot
        groups[i]=calloc(1, sizeof(struct ext2_group_desc));
        read(fd, groups[i], sizeof(struct ext2_group_desc)); //read block group
    }

    struct ext2_inode inode;

    GetInode(fd, 2, &inode); //get root folder

    ReadDirectory(&inode);
    printf("%s", COLOR_NORMAL);
    printf("Folderow: %d, plikow: %d\n", totalDirectories, totalFiles);

    for (int i=0; i < groupCount;i++)
    {
        free(groups[i]);
    }
    free(groups);

    close(fd);
}
