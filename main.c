#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

char *separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}


void rec_readdir()
{

}


int main(int argc, char* argv[]){

    if (argc != 2)
    {
        printf("Numar invalid de argumente\n");
        exit(-1);
    }

    char *inputdirstring = argv[1];

    DIR *inputdir = opendir(inputdirstring); 

    if (!inputdir)
    {
        perror(NULL);
        exit(-1);
    }

    struct dirent *currentdir = readdir(inputdir);

    while (currentdir)
    {
        char *d_name = currentdir->d_name;

        printf("d_name = %s\n", d_name);
        
            struct stat st_stat;

            char filepath[100];

            // filepath = strcat(strcat(inputdirstring, separator()), d_name);

            sprintf(filepath, "%s%s%s", inputdirstring, separator(), d_name);

            printf("filepath=%s\n", filepath);            

            if (lstat(filepath, &st_stat) != 0)
            {
                perror(NULL);
                exit(-1);
            }    

            printf("st_dev = %ld\n", st_stat.st_dev);
            printf("st_ino = %ld\n", st_stat.st_ino);


        currentdir = readdir(inputdir);
    }

    if (closedir(inputdir) == -1)
    {
        perror(NULL);
        exit(-1);
    }

    return 0;   
}