#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Se foloseste asa: %s comanda argumente\n", argv[0]);
        exit(1);
    }

    char *path = getenv("PATH");
    //printf("%s\n", path);

    //creez path-ul comenzii
    char comanda[100], temp[100];
    comanda[0] = '/';
    strcpy(comanda + 1, argv[1]);
    
    char *p = strtok(path, ":");
    while (p) {
        strcpy(temp, p);
        strcat(temp, comanda);

        //incerc sa execut execv
        execv(temp, argv + 1);
        //perror("Exec");

        p = strtok(NULL, ":");
    }
    return 0;
}