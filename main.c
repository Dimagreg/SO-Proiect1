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
#include <sys/wait.h>

#define MAX_CHR_PATH 256

int verbose_option = 0;
char safe_directory_option[MAX_CHR_PATH] = "/"; // "/" - default safe directory if not set

struct file
{
    char filepath[MAX_CHR_PATH];
    struct stat st_stat;
};

char *separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}

void writeToFileBinary(char* filename, struct file **st_file_current, int n) {

    // open file in write-only, create if doesnt exist, truncate
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        perror("open error");
        exit(-1);
    }

    ssize_t bytes_written = write(fd, *st_file_current, n * sizeof(struct file));

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

struct file* readFromFileBinary(int *st_file_src_count, char* filename) {
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
    *st_file_src_count = file_size / sizeof(struct file);

    struct file* st_file = (struct file*) malloc(*st_file_src_count * sizeof(struct file));
    if (st_file == NULL) {
        perror("malloc error");
        close(fd);
        
        exit(-1);
    }

    ssize_t bytes_read = read(fd, st_file, *st_file_src_count * sizeof(struct file));
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

void rec_readdir(int *st_file_current_count, struct file **st_file_current, DIR *dir, char *filepath, int *malicious)
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

        //check file for null permissions amd execute script to check for malware
        if ((st_stat.st_mode & S_IRWXU) == 0 &&
        (st_stat.st_mode & S_IRWXG) == 0 &&
        (st_stat.st_mode & S_IRWXO) == 0)
        {
            if (verbose_option)
            {
                printf("The file %s has no permissions.\n", filename);
            }

            pid_t pid;
            char exec_output_string[256];
            int pfd[2];

            if (pipe(pfd) < 0)
            {
                perror("pipe error\n");
                exit(1);
            }

            if ((pid = fork()) < 0)
            {
                perror("fork error\n");
                exit(1);
            }
            if (pid == 0)
            {
                //slave code
                close(pfd[0]); //close read pipe -> writes in pipe
                
                if (dup2(pfd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }

                char *args[] = {"./verify_malicious.sh", filename, NULL};
                execvp(args[0], args);

                //if reached exec has failed
                printf("Exec Failed.\n");
                exit(1);
            }
            //master code

            close(pfd[1]); //close write pipe -> reads from pipe

            FILE *stream = fdopen(pfd[0], "r");

            fscanf(stream, "%s", exec_output_string);
            
            close(pfd[0]); /* la sfarsit inchide si capatul utilizat */

            int status;
            
            waitpid(pid, &status, WUNTRACED);

            printf("exec_output_string = %s\n", exec_output_string);

            // check exec results -> "SAFE" - do nothing, [FILENAME] - move to safe directory
            if (strcmp(exec_output_string, "SAFE") != 0)
            {
                (*malicious)++;

                printf("malicious = %d\n", *malicious);

                //check if safe directory ends in '/', if not add one
                if (safe_directory_option[strlen(safe_directory_option) - 1] != separator()[0])
                {
                    strcat(safe_directory_option, separator());
                }

                strcat(safe_directory_option, exec_output_string);

                //move file to safe directory
                if (rename(filename, safe_directory_option) < 0)
                {
                    perror("rename error\n");
                    exit(1);
                }
            }

            if (verbose_option)
            {
                printf("PID %d terminated with exit code %d\n", pid, status);
            }
        }

        else
        {
            //save file in st_file
            if (!(*st_file_current = realloc(*st_file_current, (*st_file_current_count + 1) * sizeof(struct file))))
            {
                perror("rec_readdir");
                closedir(dir);
                free(*st_file_current);
                exit(-1);
            }
            
            strcpy((*st_file_current)[*st_file_current_count].filepath, filename);
            
            (*st_file_current)[*st_file_current_count].st_stat = st_stat;

            (*st_file_current_count)++;
        }
        
        //check if file is directory
        if (S_ISDIR(st_stat.st_mode))
        {
            //check if not . and ..
            if (strcmp(st_dirent->d_name, ".") != 0 && strcmp(st_dirent->d_name, "..") != 0)
            {
                DIR *inputdir = opendir(filename);

                rec_readdir(st_file_current_count, st_file_current, inputdir, filename, malicious);
            } 
        }

        st_dirent = readdir(dir);
    }

    if (closedir(dir) == -1)
    {
        perror("closedir");
        free(st_file_current);
        exit(-1);
    }
}

void compare_snapshots(struct file *st_file_current, struct file *st_file_src, int st_file_current_count, int st_file_src_count)
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
    while((opt = getopt(argc, argv, "o:d:s:v")) != -1)  
    {  
        switch(opt)  
        {  
            case 'v':
                //verbose mode on snapshot creation, child exit
                verbose_option = 1;
                break;
            case 's':
                //safe directory for malicious files
                strcpy(safe_directory_option, optarg);
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

    struct pids{
        pid_t pid;
        int malicious_files;
    } pids[10];

    int pids_count = 0;

    pid_t pid;

    for (int i = 0; i < count_of_directories; i++)
    {
        if ((pid = fork()) < 0)
        {
            perror("fork error");
            exit(1);
        }
        if (pid != 0)
        {
            //master code
            //save pid of slave, continue to next dir
            pids[pids_count].pid = pid;
            pids_count++;
            continue;
        }

        //slave code
        //execute and quit if slave

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

        //st_file of current directory state
        struct file *st_file_current = NULL;

        //st_file of snapshot
        struct file *st_file_src = NULL;

        //elements count
        int st_file_current_count = 0;
        int st_file_src_count = 0;

        // read current directory structure
        rec_readdir(&st_file_current_count, &st_file_current, inputdir, inputdirstring, &pids[pids_count].malicious_files);

        // read snapshot file if exists
        st_file_src = readFromFileBinary(&st_file_src_count, snapshot_filename);

        // no snapshot file -> nothing to compare, create new snapshot
        if (st_file_src == NULL)
        {
            writeToFileBinary(snapshot_filename, &st_file_current, st_file_current_count);
        }
        // compare snapshots
        else
        {
            compare_snapshots(st_file_current, st_file_src, st_file_current_count, st_file_src_count);

            //write new version
            writeToFileBinary(snapshot_filename, &st_file_current, st_file_current_count);
        }

        if (verbose_option)
        {
            printf("Snapshot for directory %s created succesfully.\n", inputdirstring);
        }

        printf("rec pids[%d].malicious = %d\n", pids_count, pids[pids_count].malicious_files);

        //free memory
        free(st_file_current);
        free(st_file_src);

        st_file_current = NULL;
        st_file_src = NULL;

        st_file_current_count = 0;
        st_file_src_count = 0;

        exit(0);
    }

    //master -> wait for slaves
    if (pid != 0)
    {
        for (int i = 0; i < pids_count; i++)
        {
            int status;

            printf("malicious[%d] = %d\n", i, pids[i].malicious_files);
            
            waitpid(pids[i].pid, &status, WUNTRACED);   

            printf("malicious[%d] = %d\n", i, pids[i].malicious_files);

            printf("pids_count = %d\n", pids_count);

            int es = 0;

            if (WIFEXITED(status)) 
            {
                es = WEXITSTATUS(status);
            }

            if (verbose_option)
            {
                printf("Child process %d terminated with PID %d and exit code %d. Found %d malicious files\n", i, pids[i].pid, es, pids[i].malicious_files);
            }
        }
    }

    return 0;   
}