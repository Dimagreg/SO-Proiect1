# SO-Proiect1
The subject of the project is the monitoring of changes in the directories over time, by taking snapshots at the user's request.

Requirements:
* The user will be able to specify the directory to be monitored as an argument in the command line, and the program will track the changes that appear in it and in its subdirectories.
* At each run of the program, the snapshot of the directory will be updated, storing the metadata of each entry.

# Documentation
Code works by opening the input directories recursively and collect files metadata. Then it will open snapshot file if exists and compare the file metadata between them. Supports file modification, permision modification, file added, file deleted. Directories are also treated as a file. It will keep the metadata of [dot] and [dot dot] directories but will not show any modifications on them. Comparisons are shown on screen and after a new snapshot will be created. Directories are processed in parralel. Infected files* are put in a safe directory before processed.

* File metadata is saved in a following static structure,
```
#define MAX_CHR_PATH 256

struct file
{
    char filepath[MAX_CHR_PATH];
    struct stat st_stat;
};
```

* Code should work* also in Windows system, the main issue being file separators which were resolved using this constant:
```
char *separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}
```
* The main program will start by parsing arguments given by the user, following being:
1. -o output directory where snapshot files are saved (if not set, default is working directory)
2. -s safe directory where malicious files are contained (**required!**)
3. -d directories to capture, max 10 (**should be put as last argument!**)
4. -v verbose mode for snapshot creations and forks management

* After arguments are processed, each directory entry is processed in parralel using **fork()**. Following recursive function then is called:
```
// read current directory structure
rec_readdir(&st_file_current_count, &st_file_current, inputdir, inputdirstring, &pids[pids_count].malicious_files);
```
which will read every file from passed directory, verify for *infected files in following way:
1. file has null permissions
2. file has non-ascii characters
3. file has less than 3 lines, words greater than 1000, chars greater than 2000

Options 2 and 3 are processed using a bash script **/verify_malicious.sh** inside another **fork()** which will use **pipe** to print file name if file is infected and *SAFE* if it's clean. If an infected file is found ```pids[pids_count].malicious_files``` is incremented. If file is not infected, file metadata using **lstat()** will be saved in ```st_file_current```

* Existing snapshot files in output directory will be read using ```struct file* readFromFileBinary(int *st_file_src_count, char* filename)``` and then compared with current directories state. Messages with modified files will be printed to stdout and then current snapshot will overwrite the old one with function ```void writeToFileBinary(char* filename, struct file **st_file_current, int n)```

* Directory proccessing fork will call **exit()** with the amount of infected files found. At final, each child will be closed using **waitpid()** where **status** is used to print to stdout amount of infected files found in that directory.

# Run
On first run no comparisons are made because snapshot file does not exist. Modify some files and run the second time on same directories to see changes.

`gcc -Wall -o prog.x main.c; ./prog.x -v -s [safe_dir] -o [output_directory] -d [dir1] [dir2] ...`

# Windows
Theoretically the program can be run on windows if unix-like commands and bash files are supported. This can be achieved by downloading unix terminals for windows such as [Cygwin64 Terminal](https://www.cygwin.com) 
