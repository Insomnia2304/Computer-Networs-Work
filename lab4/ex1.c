#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#define MSG1 "Eu sunt tatal"
#define MSG2 " si eu sunt copilul"

int main() {
    int sockp[2];
    char msg[1024];
    int readBytes;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }
    
    switch(fork()) {
        case -1: perror("fork"); exit(EXIT_FAILURE);
        case  0: //copil
            close(sockp[0]);
            readBytes = read(sockp[1], msg, 1023);
            if (readBytes >= 0) {
                msg[readBytes] = 0;
                write(sockp[1], strcat(msg, MSG2), readBytes + sizeof(MSG2));
            }
            close(sockp[1]);
            break;
        default: //tatal
            close(sockp[1]);
            printf("Transmit copilului: %s\n", MSG1);
            write(sockp[0], MSG1, sizeof(MSG1));
            readBytes = read(sockp[0], msg, 1023);
            close(sockp[0]);
            if (readBytes >= 0) {
                msg[readBytes] = 0;
                printf("Mesajul final: %s\n", msg); 
            }
    }
    return 0;
}