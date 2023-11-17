#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <utmp.h>
#include <time.h>
#include <sys/socket.h>

#define NMAX 1024

int clientsPID[NMAX], nbCLients, currentClientPos; //maximum of NMAX clients, can be changed
bool loggedIN[NMAX] = {0}; //every client starts logged out 
//basically a C++-like map
//it was needed for a map because maximum value of a PID can go up to 1 << 22
//a characteristic vector wouldn't work with such size so I need (int, bool) pairs, e.g (key, value)

void sighandler() {}
int makeFIFO(const char* name, int o_flag);

void loginCommand(char userInput[], char commandOutput[]);
bool checkIfUsernameExists(char username[]);
void getloggedusersCommand(char userInput[], char commandOutput[]);
void getprocinfoCommand(char userInput[], char commandOutput[]);
void logoutCommand(char userInput[], char commandOutput[]);
void quitCommand(char userInput[], char commandOutput[]);

int findClient(int PID);
void removeClient(int clientPosition);

int main() {
    signal(SIGUSR1, sighandler);

    //check if fifos already exist; if so, delete them (we want fresh fifos)
    if (access("ServerToClient", F_OK) == 0) remove("ServerToClient");
    if (access("ClientToServer", F_OK) == 0) remove("ClientToServer");

    FILE *fd_serverTempFile;
    fd_serverTempFile = fopen("serverTempFile.txt", "w");

    pid_t serverPID = getpid();
    fprintf(fd_serverTempFile, "%d", serverPID);
    fflush(fd_serverTempFile);
    fprintf(stdout, "Server started (PID: %d), waiting for clients...\n", serverPID);
    
    int fd_ServerToClient = makeFIFO("ServerToClient", O_WRONLY);
    int fd_ClientToServer = makeFIFO("ClientToServer", O_RDONLY);

    int pipe_ChildrentoFather[2];
    if (pipe(pipe_ChildrentoFather) == -1) { perror("pipe"); exit(EXIT_FAILURE); }

    while (1) {
         printf("Sleeping...\n-----------------\n");
         pause();
         
         //find clientPID
         int PID;
         read(fd_ClientToServer, &PID, sizeof(PID));
         currentClientPos = findClient(PID);
         if (currentClientPos == -1) { currentClientPos = nbCLients; clientsPID[nbCLients++] = PID; loggedIN[nbCLients - 1] = 0;}

         char userInput[NMAX], commandOutput[NMAX];
         int bytesRead;

         bytesRead = read(fd_ClientToServer, userInput, NMAX - 1);
         if (bytesRead > 0) userInput[bytesRead] = 0;
         printf("bytes read: %d, user input: %s", bytesRead, userInput);
         
         commandOutput[0] = 0;

         switch (fork()) {
            case -1: perror("fork"); break;
            case  0:
                close(pipe_ChildrentoFather[0]);

                //find the inputted command
                char command[64];
                int lg;
                for (lg = 0; userInput[lg] && userInput[lg] != '\n' && userInput[lg] != ' '; lg++)
                     command[lg] = userInput[lg];
                command[lg] = 0;

                if      (strcmp(command, "login") == 0) loginCommand(userInput, commandOutput);
                else if (strcmp(command, "get-logged-users") == 0) getloggedusersCommand(userInput, commandOutput);
                else if (strcmp(command, "get-proc-info") == 0) getprocinfoCommand(userInput, commandOutput);
                else if (strcmp(command, "logout") == 0) logoutCommand(userInput, commandOutput);
                else if (strcmp(command, "quit") == 0) quitCommand(userInput, commandOutput);
                else strcpy(commandOutput, "unknown command");

                write(pipe_ChildrentoFather[1], commandOutput, strlen(commandOutput));
                close(pipe_ChildrentoFather[1]);
                exit(EXIT_SUCCESS);
            default:    
                wait(0);

                bytesRead = read(pipe_ChildrentoFather[0], commandOutput, NMAX);
                if (bytesRead > 0) commandOutput[bytesRead] = 0;

                //would be pretty unpleasant to transfer all login data through the unnamed pipes when I can simply do this instead
                if (strcmp(commandOutput, "logged in succesfully") == 0) loggedIN[currentClientPos] = 1;
                if (strcmp(commandOutput, "logged out succesfully") == 0) loggedIN[currentClientPos] = 0;
                if (strcmp(commandOutput, "quitted") == 0) removeClient(currentClientPos);


                printf("logged in: %d, output bytes: %d, command output: %s\n-----------------\n", loggedIN[currentClientPos], bytesRead, commandOutput);
                write(fd_ServerToClient, commandOutput, strlen(commandOutput));
         }
    }

    close(pipe_ChildrentoFather[0]);
    close(pipe_ChildrentoFather[1]);
    remove("serverTempFile.txt");
    remove("ServerToClient");
    remove("ClientToServer");
    return 0;
}

int makeFIFO(const char* name, int o_flag) { 
    if (mkfifo(name, 0777) == -1) { 
        perror("mkfifo");
        exit(EXIT_FAILURE); 
    }
    int fd;
    if ((fd = open(name, o_flag)) == -1) {
        perror("open fifo");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void loginCommand(char userInput[], char commandOutput[]) {
    if (loggedIN[currentClientPos] == 1) { sprintf(commandOutput + strlen(commandOutput), "already logged in"); return; }

    char *command = strtok(userInput, " \n");

    //doua puncte
    char *colon = strtok(NULL, " \n");
    if (colon == 0 || colon[0] != ':' || colon[1] != 0) { sprintf(commandOutput + strlen(commandOutput), "[bad placed/missing colon] usage: login : username"); return; }

    char *arg = strtok(NULL, " \n");
    if (arg == 0) { sprintf(commandOutput + strlen(commandOutput), "[missing argument] usage: login : username"); return; }

    char *tooManyArgs = strtok(NULL, " \n");
    if (tooManyArgs != NULL) { sprintf(commandOutput + strlen(commandOutput), "[too many arguments] usage: login : username"); return; }

    if (checkIfUsernameExists(arg) == 1) { sprintf(commandOutput + strlen(commandOutput), "logged in succesfully"); return; }
    sprintf(commandOutput + strlen(commandOutput), "username not registred");
}

bool checkIfUsernameExists(char username[]) {
    FILE *configFile;
    if ((configFile = fopen("config.txt", "r")) == NULL) { perror("open config file"); return false; }

    char entry[NMAX];
    while (fgets(entry, NMAX, configFile) != NULL) {
        if (entry[strlen(entry) - 1] == '\n') entry[strlen(entry) - 1] = 0;
        if (strcmp(entry, username) == 0) return true;
    }
    return false;
}

void getloggedusersCommand(char userInput[], char commandOutput[]) {
    if (loggedIN[currentClientPos] == 0) { sprintf(commandOutput + strlen(commandOutput), "you must login to use this command"); return; }

    char *command = strtok(userInput, " \n");
    char *tooManyArgs = strtok(NULL, " \n");
    if (tooManyArgs != NULL) { sprintf(commandOutput + strlen(commandOutput), "[too many arguments] usage: get-logged-users"); return; }

    sprintf(commandOutput + strlen(commandOutput), "users authenticated on the OS:\n");
    int index = 1;

    struct utmp *utmpEntry;
    setutent(); //open utmp file

    while ((utmpEntry = getutent()) != NULL) {
        if (utmpEntry->ut_type == USER_PROCESS) {
            time_t loginTime = utmpEntry->ut_time;
            sprintf(commandOutput + strlen(commandOutput), "%d. username: %s, hostname for remote login: %s, time entry was made: %s",
                    index,
                    utmpEntry->ut_user,
                    utmpEntry->ut_host,
                    ctime(&loginTime));
            index++;
        }
    }
    //remove last newline added by ctime function (output formatting purpose)
    if (commandOutput[strlen(commandOutput) - 1] == '\n') commandOutput[strlen(commandOutput) - 1] = 0;

    endutent(); //close current opened utmp file
}

void getprocinfoCommand(char userInput[], char commandOutput[]) {
    if (loggedIN[currentClientPos] == 0) { sprintf(commandOutput + strlen(commandOutput), "you must login to use this command"); return; }

    char *command = strtok(userInput, " \n");

    //doua puncte
    char *colon = strtok(NULL, " \n");
    if (colon == 0 || colon[0] != ':' || colon[1] != 0) { sprintf(commandOutput + strlen(commandOutput), "[bad placed/missing colon] usage: get-proc-info : pid"); return; }

    char *pid = strtok(NULL, " \n");
    if (pid == 0) { sprintf(commandOutput + strlen(commandOutput), "[missing pid] usage: get-proc-info : pid"); return; }

    char *tooManyArgs = strtok(NULL, " \n");
    if (tooManyArgs != NULL) { sprintf(commandOutput + strlen(commandOutput), "[too many arguments] usage: get-proc-info : pid"); return; }

    int sockp[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) == -1) { sprintf(commandOutput + strlen(commandOutput), "socket error"); return; }

    //not needed, but HAD to use socketpair! :)
    switch (fork()) {
        case -1: { sprintf(commandOutput + strlen(commandOutput), "fork error"); return; }
        case  0: 
            close(sockp[0]);
            
            char serverAnswer[NMAX];
            serverAnswer[0] = 0;

            char PIDStatusFile[NMAX];
            sprintf(PIDStatusFile, "/proc/%s/status", pid);
            

            FILE *fProcStatus;
            if ((fProcStatus = fopen(PIDStatusFile, "r")) == NULL) {
                printf("%s\n", PIDStatusFile);
                sprintf(serverAnswer + strlen(serverAnswer), "unexistent process/unable to open status file of specified pid");
            }  
            else {
                char line[NMAX];
                bool hasVmSize = 0;

                while (fgets(line, NMAX, fProcStatus) != NULL) { 
                    if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = 0; //remove newlines from fgets
                    if (strstr(line, "Name:") != NULL) sprintf(serverAnswer + strlen(serverAnswer), "name: %s, ", line + strlen("Name:") + 1);
                    if (strstr(line, "State:") != NULL) sprintf(serverAnswer + strlen(serverAnswer), "state: %s, ",line + strlen("State:") + 1);
                    if (strstr(line, "PPid:") != NULL) sprintf(serverAnswer + strlen(serverAnswer), "ppid: %s, ", line + strlen("PPid:") + 1);
                    if (strstr(line, "Uid:") != NULL) sprintf(serverAnswer + strlen(serverAnswer), "uid: %s, ", line + strlen("Uid:") + 1);
                    if (strstr(line, "VmSize:") != NULL) { sprintf(serverAnswer + strlen(serverAnswer), "vmsize: %s", line + strlen("VmSize:") + 1); hasVmSize = 1; }
                }
                if (hasVmSize == 0) sprintf(serverAnswer + strlen(serverAnswer), "vmsize: N/A");
            }

            write(sockp[1], serverAnswer, strlen(serverAnswer));
            close(sockp[1]);
            exit(EXIT_SUCCESS);
        default: 
            close(sockp[1]);

            int bytesRead;
            if ((bytesRead = read(sockp[0], commandOutput, NMAX - 1)) == -1) { sprintf(commandOutput, "read from socket error"); return; }
            close(sockp[0]);
            commandOutput[bytesRead] = 0;
    }
}

void logoutCommand(char userInput[], char commandOutput[]) {
    if (loggedIN[currentClientPos] == 0) { sprintf(commandOutput + strlen(commandOutput), "already logged out"); return; }

    char *command = strtok(userInput, " \n");
    char *tooManyArgs = strtok(NULL, " \n");
    if (tooManyArgs != NULL) { sprintf(commandOutput + strlen(commandOutput), "[too many arguments] usage: logout"); return; }

    sprintf(commandOutput + strlen(commandOutput), "logged out succesfully");    
}

void quitCommand(char userInput[], char commandOutput[]) {
    char *command = strtok(userInput, " \n");
    char *tooManyArgs = strtok(NULL, " \n");
    if (tooManyArgs != NULL) { sprintf(commandOutput + strlen(commandOutput), "[too many arguments] usage: quit"); return; }

    sprintf(commandOutput + strlen(commandOutput), "quitted");    
}

int findClient(int PID) {
    for (int i = 0; i < nbCLients; i++)
        if (clientsPID[i] == PID)
            return i;
    return -1;
}

void removeClient(int clientPosition) {
    for (int i = clientPosition; i < nbCLients - 1 ; i++) {
        clientsPID[i] = clientsPID[i + 1];
        loggedIN[i] = loggedIN[i + 1];
    }   
    nbCLients--;
}
