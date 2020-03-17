#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAXLINE 1024
#define backlog 10

int main(int argc, char *argv[]){
    int listenfd, connfd;
    struct sockaddr_in server;
    char buffer[MAXLINE], cmd[MAXLINE], arg[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
    char message[MAXLINE], failedcmd[MAXLINE];
    char history[backlog][100][MAXLINE];
    int serverPORT;
    int num, j;
    fd_set allset, rset;
    int client[backlog];
    int maxfd, maxClient;
    int numReady;
    int c;
    int i[backlog];
    int sum[backlog];

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
    maxfd = listenfd;
    maxClient = -1;

    for(c=0; c<backlog; c++){
        client[c] = -1;
        i[c] = 0;
        sum[c] = 0;
    }

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for(;;){
        rset = allset;
        numReady = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset)){
            connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
            for(c=0; c<backlog; c++){
                if(client[c] < 0){
                    client[c] = connfd;
                    break;
                }
            }
            if(c == 10){
                printf("Too many client\n");
                exit(1);
            }
            FD_SET(connfd, &allset);
            if(connfd > maxfd) maxfd = connfd;
            if(c > maxClient) maxClient = c;

            numReady--;
            if(numReady <= 0)continue;
        }

        for(c=0; c<=maxClient; c++){
            if(client[c] < 0)continue;
            if(FD_ISSET(client[c], &rset)){
                connfd = client[c];
                ssize_t len;
                len = read(connfd, &buffer, MAXLINE);
                strcpy(history[c][i[c]], buffer);
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

                    sum[c] += num;

                    sprintf(message, "### BALANCE: %d NTD\n", sum[c]);
                    write(connfd, message, sizeof(message));
                    i[c]++;
                }else if(strcmp(cmd, "WITHDRAW") == 0){
                    strcpy(arg, buffer + strlen(cmd) + 1);
                    if(arg[strlen(arg)-1] == '\n')arg[strlen(arg)-1] = '\0';
                    strcpy(arg1, strtok(arg, " "));
                    if(arg1[strlen(arg1)-1] == '\n')arg1[strlen(arg1)-1] = '\0';
                    strcpy(arg2, buffer + strlen(cmd) + strlen(arg1) + 2);
                    if(arg2[strlen(arg2)-1] == '\n')arg2[strlen(arg2)-1] = '\0';

                    num = atoi(arg1);
                    if(strcmp(arg2, "USD") == 0) num = num * 30;

                    if(sum[c] - num < 0){
                        strcpy(message, "!!! FAILED: Not enough money in the account\n");
                        write(connfd, message, sizeof(message));
                        sprintf(message, "### BALANCE: %d NTD\n", sum[c]);
                        write(connfd, message, sizeof(message));
                        char temp[MAXLINE];
                        strcpy(temp, history[c][i[c]]);
                        if(temp[strlen(temp)-1] == '\n')temp[strlen(temp)-1] = ' ';
                        sprintf(failedcmd, "%s(FAILED)\n", temp);
                        strcpy(history[c][i[c]], failedcmd);
                        i[c]++;
                        continue;
                    }
                    sum[c] -= num;
                    sprintf(message, "### BALANCE: %d NTD\n", sum[c]);
                    write(connfd, message, sizeof(message));
                    i[c]++;
                }else if(strcmp(cmd, "HISTORY") == 0){
                    if(i[c] == 0){
                        strcpy(message, "%%% No commands received so far\n");
                        write(connfd, message, sizeof(message));
                    }else{
                        for(j=0; j<i[c]; j++){
                            sprintf(message, "### %s", history[c][j]);
                            write(connfd, message, sizeof(message));
                        }
                    }
                    sprintf(message, "$$$ BALANCE: %d NTD\n", sum[c]);
                    write(connfd, message, sizeof(message));
                }else if(strcmp(cmd, "EXIT") == 0){
                    close(connfd);
                    i[c] = 0;
                    sum[c] = 0;
                    FD_CLR(connfd, &allset);
                    client[c] = -1;
                }else{
                    printf("???\n");
                }
                numReady--;
                if(numReady <= 0)break;
            }
        }
    }
}