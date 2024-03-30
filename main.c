#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define MAX_CHR_PATH 256

struct file
{
    char filepath[MAX_CHR_PATH];
    struct stat st_stat;
};

struct file *st_file;
int file_count = 0;

char *separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}

void rec_readdir(DIR *dir, char *filepath)
{
    struct dirent *st_dirent = readdir(dir);

    //read each entry of the directory in st_dirent
    while (st_dirent)
    {
        struct stat st_stat;
        char filename[MAX_CHR_PATH];
        
        sprintf(filename, "%s%s%s", filepath, separator(), st_dirent->d_name);

        printf("filename=%s\n", filename);            

        if (lstat(filename, &st_stat) != 0)
        {
            perror(NULL);
            exit(-1);
        }    

        //save file in st_file
        if (!(st_file = realloc(st_file, (file_count + 1) * sizeof(struct file))))
        {
            perror(NULL);
            exit(-1);
        }
        
        strcpy(st_file[file_count].filepath, filename);
        
        st_file[file_count].st_stat = st_stat;

        file_count++;

        //check if file is directory
        if (S_ISDIR(st_stat.st_mode))
        {
            //check if not . and ..
            if (strcmp(st_dirent->d_name, ".") != 0 && strcmp(st_dirent->d_name, "..") != 0)
            {
                DIR *inputdir = opendir(filename);

                rec_readdir(inputdir, filename);
            } 
        }

        st_dirent = readdir(dir);
    }

    if (closedir(dir) == -1)
    {
        perror(NULL);
        exit(-1);
    }
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

    rec_readdir(inputdir, inputdirstring);

    return 0;   
}