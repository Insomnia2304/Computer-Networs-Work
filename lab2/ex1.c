#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

void sighandler1() {
    printf("Am primit: lost!\n");
}

void sighandler2() {
    printf("Am primit: fortune!\n");
}

int main() {
    pid_t pid;
    signal(SIGUSR1, sighandler1);
    signal(SIGUSR2, sighandler2);

    if ((pid = fork()) < 0) {
        perror("Eroare la fork");
        exit(1);
    }
    if (pid == 0) {
        if (getpid() % 2) waitpid(getppid(), NULL, 0);
        printf("Sunt fiul si am pid-ul %d!\n", getpid());
        exit(0);
    }
    if (pid > 0) {
        if (pid % 2) { kill(pid, SIGUSR1); kill(getpid(), SIGKILL); }
        else kill(pid, SIGUSR2);
        printf("Tatal: Traieeesc!\n");
    }
    return 0;
}