#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#define NMAX 1024

char opStack[NMAX], lgOp = 0;
int numbersStack[NMAX], lgNb = 0;
char opPrecedence[] = "*/+-(";
int prededence[] = {0, 0, 1, 1, 2}; //C++-like map


char expr[1024];

void buildExpr(int sockp[]) {
    switch (fork()) {
        case -1: perror("fork"); exit(EXIT_FAILURE);
        case 0:
            close(sockp[0]);
            int firstNb = numbersStack[lgNb - 2];
            int secondNb = numbersStack[lgNb - 1];
            char op = opStack[lgOp - 1];
            //printf("-->Operand: %c\n", op);
            //printf("-->Operatori: %d, %d\n", firstNb, secondNb);
            int ans;
            switch (op) {
                case '*': ans = firstNb * secondNb; break;
                case '/': ans = firstNb / secondNb; break;
                case '+': ans = firstNb + secondNb; break;
                case '-': ans = firstNb - secondNb; break;
            }
            if (write(sockp[1], &ans, sizeof(ans)) == -1) { perror("write in child"); exit(EXIT_FAILURE); }
            //printf("-->%d\n", ans);
            close(sockp[1]);
            exit(EXIT_SUCCESS);
        default: //citesc si pun inapoi in stiva de numere
            lgNb -= 2; lgOp -= 1;
            if (read(sockp[0], &numbersStack[lgNb++], sizeof(numbersStack[0])) == -1) { perror("read in father"); exit(EXIT_FAILURE); }
    } 
}

int makeNumber(int *pos) {
    char digits[] = "0123456789";
    int number = 0; 
    for ( ; expr[*pos] && strchr(digits, expr[*pos]) != NULL; (*pos)++) {
            number = number * 10 + expr[*pos] - '0';
    }
    (*pos)--;
    //printf("%d\n", number);
    return number;
}

int main() {
    int sockp[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) == -1) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    printf("Introduceti o expresie aritmetica formata doar din numere naturale si caracterele \"+-*/()\", FARA spatii: ");
    //gets(expr);
    fgets(expr, 1024, stdin);
    expr[strlen(expr) - 1] = 0; //sterg newline-ul


    for (int i = 0; expr[i]; i++) {
         switch(expr[i]) {
            case ' ': break;
            case '(': opStack[lgOp++] = expr[i]; break;
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
            case '9': numbersStack[lgNb++] = makeNumber(&i); break; 
            case ')': while (opStack[lgOp - 1] != '(') buildExpr(sockp); lgOp--; break;
            default: //+-*/
                while (lgOp > 0 && prededence[strchr(opPrecedence, opStack[lgOp - 1]) - opPrecedence] <= prededence[strchr(opPrecedence, expr[i]) - opPrecedence]) 
                       buildExpr(sockp);
                opStack[lgOp++] = expr[i];
         }
    }
    while(lgNb > 1) buildExpr(sockp);
    close(sockp[0]); 
    close(sockp[1]);
    printf("Rezultatul final este: %d!\n", numbersStack[0]);
    return 0;
}
