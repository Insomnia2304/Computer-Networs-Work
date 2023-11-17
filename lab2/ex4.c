#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

void sigusr1(int signum) {
    FILE* f;
    f = fopen("ex4.txt", "a");
    perror("File open");
    fprintf(f, "Am primit semnal.\n");
    fclose(f);
}

void sigint(int signum) {
    printf("Haha, mai incearca!\n");
}

int main(int argv, char **argc) {
    signal(SIGUSR1, sigusr1);
    signal(SIGINT, sigint);
    pid_t pid = getpid();
    for (int i = 1; ; i++) {
        printf("%d: PID-ul procesului %s este: %d! Acum, asteptam 3 secunde...\n", i, argc[0], pid);
        if (i == 21) signal(SIGINT, SIG_DFL);
        sleep(3);
    }
    return 0;
}