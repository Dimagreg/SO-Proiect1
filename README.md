# SO-Proiect1
Subiectul proiectului este monitorizarea modificarilor aparute in directoare de-a lungul timpului, prin realizarea de capturi (snapshots) la cererea utilizatorului.

Cerintele:
* Utilizatorul va putea specifica directorul de monitorizat ca argument in linia de comanda, iar programul va urmari schimbarile care apar in acesta si in subdirectoarele sale.
* La fiecare rulare a programului, se va actualiza captura (snapshot) directorului, stocand metadatele fiecarei intrari.

# Implementation
Code works by opening the input directory recursively and collect files metadata. Then it will open snapshot file if exists and compare the file metadata between them. Supports file modification, permision modification, file added, file deleted. Directory is also treated as a file. It will keep the metadata of dot and dot dot directories but will not show any modifications on them. Comparisons are shown on screen and after a new snapshot will be created.

# Run
On first run no comparisons are made because snapshot file does not exist. Modify some files and run the second time on same directory to see changes.

`gcc -Wall -o prog.x main.c; ./prog.x [directory]`