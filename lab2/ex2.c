#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

int main() {
    pid_t pid;
    switch (pid = fork()) {
    case -1: 
        perror("Fork");
        exit(EXIT_FAILURE);
    case 0: //copilul
        char *argList[] = {"ls", "-a", "-l", NULL};
        execvp(argList[0], argList);
        perror("Execvp");
    default:
        int stat;
        if ( wait(&stat) == -1) perror("Wait");
        printf("Copilul a iesit cu statusul: %d!\n", WEXITSTATUS(stat));
    }
    return 0;
}