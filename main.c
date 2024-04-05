//MAKE SURE TO RUN IT ONCE TO CREATE A SNAPSHOT FILE
//THEN IT WILL START TO COMPARE IT TO OLDER SNAPSHOT ON MODIFIED DIRECTORY

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define MAX_CHR_PATH 256

struct file
{
    char filepath[MAX_CHR_PATH];
    struct stat st_stat;
};

//st_file of current directory state
struct file *st_file_current;

//st_file of snapshot
struct file *st_file_src;

//elements count
int st_file_current_count = 0;
int st_file_src_count = 0;

char *separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}

void writeToFileBinary(char* filename, struct file* st_file, int n) {

    // open file in write-only, create if doesnt exist, truncate
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        perror("open error");
        exit(-1);
    }

    ssize_t bytes_written = write(fd, st_file, n * sizeof(struct file));

    if (bytes_written == -1) {
        perror("write error");
        close(fd);
        exit(-1);
    }

    if (close(fd) == -1) {
        perror("close error");
        exit(-1);
    }

    //printf("Snapshot created successfully\n");
}

struct file* readFromFileBinary(char* filename) {
    // open with read only
    int fd = open(filename, O_RDONLY);

    // return NULL if file doesn't exist
    if (fd == -1) {

        return NULL;
    }

    // get file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek error");
        close(fd);
        
        exit(-1);
    }

    // return to start
    off_t offset = lseek(fd, 0, SEEK_SET);
    if (offset == -1) {
        perror("lseek set error");
        close(fd);
        
        exit(-1);
    }

    // number of elements of directory
    st_file_src_count = file_size / sizeof(struct file);

    struct file* st_file = (struct file*) malloc(st_file_src_count * sizeof(struct file));
    if (st_file == NULL) {
        perror("malloc error");
        close(fd);
        
        exit(-1);
    }

    ssize_t bytes_read = read(fd, st_file, st_file_src_count * sizeof(struct file));
    if (bytes_read == -1) {
        perror("read error");
        free(st_file);
        close(fd);
        
        exit(-1);
    }

    // close fd
    if (close(fd) == -1) {
        perror("close fd error");
        free(st_file);

        exit(-1);
    }

    return st_file;
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

        if (lstat(filename, &st_stat) != 0)
        {
            perror(NULL);
            closedir(dir);
            exit(-1);
        }    

        //save file in st_file
        if (!(st_file_current = realloc(st_file_current, (st_file_current_count + 1) * sizeof(struct file))))
        {
            perror(NULL);
            closedir(dir);
            free(st_file_current);
            exit(-1);
        }
        
        strcpy(st_file_current[st_file_current_count].filepath, filename);
        
        st_file_current[st_file_current_count].st_stat = st_stat;

        st_file_current_count++;

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
        free(st_file_current);
        exit(-1);
    }
}

void compare_snapshots(struct file *st_file_current, struct file *st_file_src, int st_file_current_count, int st_file_current_src)
{
    struct stat st_stat_current;

    int file_found = 0;

    //compare st_file_src to st_file_current
    //show modified files
    //show files which were not found -> deleted.
    for (int i = 0; i < st_file_src_count; i++)
    {
        char *filepath = st_file_src[i].filepath;

        for (int j = 0; j < st_file_current_count; j++)
        {
            //find the exact file in both st
            if (strcmp(filepath, st_file_current[j].filepath) == 0)
            {
                // filter out aux directories because it modifies on every file add/delete and floods output
                if (filepath[strlen(filepath) - 1 ] != '.')
                {
                    //read lstat of file st_file_current
                    if (lstat(st_file_current[j].filepath, &st_stat_current) != 0)
                    {
                        perror(NULL);
                        exit(-1);
                    }    

                    //st_size differs -> file was modified
                    if (st_file_src[i].st_stat.st_size != st_stat_current.st_size)
                    {
                        printf("[*]\t%s - was modified\n", filepath);
                    }

                    //st_mode differs -> file permissions were modified
                    if (st_file_src[i].st_stat.st_mode != st_stat_current.st_mode)
                    {
                        printf("[*]\t%s - permissions were modified\n", filepath);
                    }

                    //st_nlink differs -> hard link count modified
                    if (st_file_src[i].st_stat.st_nlink != st_stat_current.st_nlink)
                    {
                        printf("[*]\t%s - hard link count was modified\n", filepath);
                    }
                }

                file_found = 1;
                break;
            }
        }

        //file was not found in st_file_current -> it was deleted
        if (file_found == 0 && filepath[strlen(filepath) - 1 ] != '.')
        {
            printf("[-]\t%s - was deleted\n", filepath);
        }

        //reset flag
        file_found = 0;
    }

    //compare st_file_current to st_file_src
    //show files which were not found -> added.
    for (int i = 0; i < st_file_current_count; i++)
    {
        char *filepath = st_file_current[i].filepath;

        for (int j = 0; j < st_file_src_count; j++)
        {
            //find the exact file in both st
            if (strcmp(filepath, st_file_src[j].filepath) == 0)
            {
                file_found = 1;
                break;
            }
        }

        //file was not found in st_file_current -> it was added
        if (file_found == 0 && filepath[strlen(filepath) - 1 ] != '.')
        {
            printf("[+]\t%s - was added\n", filepath);
        }

        //reset flag
        file_found = 0;
    }
}

int main(int argc, char* argv[]){

    char array_of_directories[10][MAX_CHR_PATH];
    int count_of_directories = 0;

    char snapshot_path[MAX_CHR_PATH];
    int snapshot_path_flag = 0;

    int opt;

    // get program arguments
    while((opt = getopt(argc, argv, "o:d:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'o':  
                // get snapshot location
                snapshot_path_flag = 1;
                strcpy(snapshot_path, optarg);
                break;  
            case 'd':               
                // get directory names
                optind--;

                for( ;optind < argc && *argv[optind] != '-'; optind++){
                    // exit if count of dir > 10
                    if (count_of_directories == 10)
                    {
                        printf("too many arguments for -d: max 10\n");
                        exit(-1);
                    }

                    // insert in array
                    strcpy(array_of_directories[count_of_directories], argv[optind]);

                    count_of_directories++;
                }
                
                break;
        }
    }

    printf("snapshot_path = %s\n", snapshot_path);

    for (int i = 0; i < count_of_directories; i++)
    {
        // get directory name
        char *inputdirstring = array_of_directories[i];

        // get snapshot file name
        // starts with a . , (optional: folowing snapshot location), following directory name -> .[dirname] or [dirlocation]/.[dirname]
        char snapshot_filename[MAX_CHR_PATH] = ".";

        // check if flag is 1 -> -o option is specified
        // [dirlocation]/.[dirname]
        if (snapshot_path_flag == 1)
        {
            char string[MAX_CHR_PATH] = "";

            strcat(string, snapshot_path);
            strcat(string, separator());
            strcat(string, ".");
            strcat(string, inputdirstring);

            strcpy(snapshot_filename, string);
        } 
        // .[dirname]
        else
        {   
            strcat(snapshot_filename, inputdirstring);
        }

        // open directory
        DIR *inputdir = opendir(inputdirstring); 

        // skip if not directory
        if (!inputdir)
        {
            printf("%s - is not a directory\n", inputdirstring);
            continue;
        }

        // read current directory structure
        rec_readdir(inputdir, inputdirstring);

        // read snapshot file if exists
        st_file_src = readFromFileBinary(snapshot_filename);

        // no snapshot file -> nothing to compare, create new snapshot
        if (st_file_src == NULL)
        {
            writeToFileBinary(snapshot_filename, st_file_current, st_file_current_count);
        }
        // compare snapshots
        else
        {
            compare_snapshots(st_file_current, st_file_src, st_file_current_count, st_file_src_count);

            //write new version
            writeToFileBinary(snapshot_filename, st_file_current, st_file_current_count);
        }

        //free memory
        free(st_file_current);
        free(st_file_src);

        st_file_current = NULL;
        st_file_src = NULL;

        st_file_current_count = 0;
        st_file_src_count = 0;
    }

    return 0;   
}