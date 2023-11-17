#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    printf("Waiting for a reader...\n");
    int fd;
    if ((fd = open("ex2fifo", O_WRONLY)) == -1) {
        perror("open FIFO");
        exit(EXIT_FAILURE);
    }
    printf("Finally found a reader, I will show him prog.c!\n");

    switch(fork()) {
        case -1:
            perror("fork:");
            exit(EXIT_FAILURE);
        case 0:
            close(1);
            dup(fd);
            close(fd);

            if (execlp("cat", "cat", "prog.c", NULL) == -1) {
                perror("execlp");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            waitpid(0);
            printf("prog.c's content has been put in the FIFO!\n");
    }
    return 0;
}