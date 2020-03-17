#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXLINE 1024
#define backlog 10

void sig_fork(int signo){
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
    };
    return;
}

int main(int argc, char *argv[]){
    int listenfd, connfd;
    struct sockaddr_in server;
    char buffer[MAXLINE], cmd[MAXLINE], arg[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
    char message[MAXLINE], failedcmd[MAXLINE];
    char *history[MAXLINE];
    int serverPORT;
    int num, sum, i, j;
    pid_t pid;

    if(argc != 2){
        printf("wrong input\n");
        exit(1);
    }

    serverPORT = atoi(argv[1]);
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("socket error\n");
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("bind error\n");
        exit(1);
    }

    listen(listenfd, backlog);
    signal(SIGCHLD, sig_fork);

    for(;;){
        connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
        if((pid = fork()) == 0){
            close(listenfd);
            sum = 0;
            i = 0;
            for(;;){
                ssize_t len;
                len = read(connfd, &buffer, MAXLINE);
                history[i] = malloc(MAXLINE * sizeof(char));
                strcpy(history[i], buffer);
                strcpy(cmd, strtok(buffer, " "));
                if(cmd[strlen(cmd)-1] == '\n')cmd[strlen(cmd)-1] = '\0';
                if(strcmp(cmd, "DEPOSIT") == 0){
                    strcpy(arg, buffer + strlen(cmd) + 1);
                    if(arg[strlen(arg)-1] == '\n')arg[strlen(arg)-1] = '\0';
                    strcpy(arg1, strtok(arg, " "));
                    if(arg1[strlen(arg1)-1] == '\n')arg1[strlen(arg1)-1] = '\0';
                    strcpy(arg2, buffer + strlen(cmd) + strlen(arg1) + 2);
                    if(arg2[strlen(arg2)-1] == '\n')arg2[strlen(arg2)-1] = '\0';

                    num = atoi(arg1);
                    if(strcmp(arg2, "USD") == 0) num = num * 30;

                    sum += num;

                    sprintf(message, "### BALANCE: %d NTD\n", sum);
                    write(connfd, message, sizeof(message));
                    i++;
                }else if(strcmp(cmd, "WITHDRAW") == 0){
                    strcpy(arg, buffer + strlen(cmd) + 1);
                    if(arg[strlen(arg)-1] == '\n')arg[strlen(arg)-1] = '\0';
                    strcpy(arg1, strtok(arg, " "));
                    if(arg1[strlen(arg1)-1] == '\n')arg1[strlen(arg1)-1] = '\0';
                    strcpy(arg2, buffer + strlen(cmd) + strlen(arg1) + 2);
                    if(arg2[strlen(arg2)-1] == '\n')arg2[strlen(arg2)-1] = '\0';

                    num = atoi(arg1);
                    if(strcmp(arg2, "USD") == 0) num = num * 30;

                    if(sum - num < 0){
                        strcpy(message, "!!! FAILED: Not enough money in the account\n");
                        write(connfd, message, sizeof(message));
                        sprintf(message, "### BALANCE: %d NTD\n", sum);
                        write(connfd, message, sizeof(message));
                        char temp[MAXLINE];
                        strcpy(temp, history[i]);
                        if(temp[strlen(temp)-1] == '\n')temp[strlen(temp)-1] = ' ';
                        sprintf(failedcmd, "%s(FAILED)\n", temp);
                        strcpy(history[i], failedcmd);
                        i++;
                        continue;
                    }
                    sum -= num;
                    sprintf(message, "### BALANCE: %d NTD\n", sum);
                    write(connfd, message, sizeof(message));
                    i++;
                }else if(strcmp(cmd, "HISTORY") == 0){
                    if(i == 0){
                        strcpy(message, "%%% No commands received so far\n");
                        write(connfd, message, sizeof(message));
                    }else{
                        for(j=0; j<i; j++){
                            sprintf(message, "### %s", history[j]);
                            write(connfd, message, sizeof(message));
                        }
                    }
                    sprintf(message, "$$$ BALANCE: %d NTD\n", sum);
                    write(connfd, message, sizeof(message));
                }else if(strcmp(cmd, "EXIT") == 0){
                    close(connfd);
                    for(j=0; j<i; j++){
                        free(history[i]);
                    }
                    exit(0);
                }else{
                    printf("???\n");
                }
            }
        }else{
            close(connfd);
        }
    }
}