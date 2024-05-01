# SO-Proiect1
Subiectul proiectului este monitorizarea modificarilor aparute in directoare de-a lungul timpului, prin realizarea de capturi (snapshots) la cererea utilizatorului.

Cerintele:
* Utilizatorul va putea specifica directorul de monitorizat ca argument in linia de comanda, iar programul va urmari schimbarile care apar in acesta si in subdirectoarele sale.
* La fiecare rulare a programului, se va actualiza captura (snapshot) directorului, stocand metadatele fiecarei intrari.

# Implementation
Code works by opening the input directories recursively and collect files metadata. Then it will open snapshot file if exists and compare the file metadata between them. Supports file modification, permision modification, file added, file deleted. Directories are also treated as a file. It will keep the metadata of [dot] and [dot dot] directories but will not show any modifications on them. Comparisons are shown on screen and after a new snapshot will be created. Directories are processed in parralel. Infected files are put in a safe directory before processed.

# Limitation
Code processes maximum of 10 directories at a time.

# Options
* -o output directory of snapshots (if not set, default is working directory)
* -s safe directory where malicious files are contained
* -d directories to be captured (should be put as last argument and do not put "/" at end)
* -v verbose mode for snapshot creations and forks management

# Run
On first run no comparisons are made because snapshot file does not exist. Modify some files and run the second time on same directories to see changes.

`gcc -Wall -o prog.x main.c; ./prog.x -v -s [safe_dir] -o [output_directory] -d [dir1] [dir2] ...`
