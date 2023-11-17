#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define NMAX 1024

int main() {
    //check if server is running [care: it might be still running for a bit, even if server's process was terminated] [check on terminal with ps -u]
    FILE *fd_serverTempFile = fopen("serverTempFile.txt", "r");
    pid_t serverPID, PID = getpid();
    fscanf(fd_serverTempFile, "%d", &serverPID);
    //printf("Server PID: %d\n", serverPID);

    if (kill(serverPID, 0) == -1) { 
        printf("server offline\n");
        exit(EXIT_FAILURE);
    }

    int fd_ServerToClient;
    int fd_ClientToServer;
    while ((fd_ServerToClient = open("ServerToClient", O_RDONLY)) == -1) {}
    while ((fd_ClientToServer = open("ClientToServer", O_WRONLY)) == -1) {}

    char userInput[NMAX], commandOutput[NMAX];
    int bytesRead;
           
    while (1) {
        printf("Insert any command: ");
        fgets(userInput, sizeof(userInput), stdin);

        if (write(fd_ClientToServer, &PID, sizeof(PID)) == -1) perror("write clientPID");
        if (write(fd_ClientToServer, userInput, strlen(userInput)) == -1) perror("write userInput");
        kill(serverPID, SIGUSR1);

        bytesRead = read(fd_ServerToClient, commandOutput, NMAX - 1);
        if (bytesRead > 0) {
            commandOutput[bytesRead] = 0; 
            printf("Server answer: %s\n----------------------------------------------------------------------------\n", commandOutput);
            if (strcmp(commandOutput, "quitted") == 0) exit(EXIT_SUCCESS);
        }
    }
    return 0;
}