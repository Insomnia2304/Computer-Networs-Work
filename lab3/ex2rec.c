#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {

    if (mkfifo("ex2fifo", 0777) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for someone to write in the FIFO...\n");
    int fd;
    if ((fd = open("ex2fifo", O_RDONLY)) == -1) {
        perror("open FIFO");
        exit(EXIT_FAILURE);
    }
    printf("Someone has finally written in the FIFO, now let's process it!\n");

    switch(fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            close(0);
            dup(fd);
            close(fd);

            if ((fd = open("prog2.c", O_WRONLY)) == -1) {
                perror("open prog2.c");
                exit(EXIT_FAILURE);
            }

            close(1);
            dup(fd);
            close(fd);

            if (execlp("grep", "grep", "include", NULL) == -1) {
                perror("execlp");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            waitpid(0);
            printf("Processed received information and put in prog2.c!\n");
            if (remove("ex2fifo") != 0) {
                perror("remove ex2fifo");
                exit(EXIT_FAILURE);
            }
    }
    return 0;
}
