#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <wait.h>

bool isPrime(int x) {
    if (x < 2) return false;
    for (int d = 2; d * d <= x; d++)
        if (x % d == 0) return false;
    return true;
}

"#include" sdjsald
eefdscj#includedsakdasj
#include#include
cccccc#includ

int main() {
    int fd[2];
    int x, xChild;
    char buffer[100];

    printf("Introduceti un numar: ");
    scanf("%d", &x);

    if (pipe(fd) == -1) {
        perror("Pipe");
        exit(EXIT_FAILURE);
    }
    switch(fork()) {
        case -1:
            perror("Fork:");
            exit(EXIT_FAILURE);
        case 0:
            //copilul

            read(fd[0], &xChild, sizeof(xChild));
            close(fd[0]);

            isPrime(xChild) ? write(fd[1], "yes", 3) : write(fd[1], "no", 2);
            close(fd[1]);

            break;
        default:
            //tatal

            write(fd[1], &x, sizeof(x));
            close(fd[1]);

            wait(0);
            
            int bytesRead = read(fd[0], buffer, 99);
            close(fd[0]);
            buffer[bytesRead] = 0;
            printf("%s\n", buffer);
    }
    return 0;
}