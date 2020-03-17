#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAXLINE 1024

int main(int argc, char *argv[]){
    char *serverIP;
    int serverPORT;
    int sockfd;
    struct sockaddr_in server;
    char buffer[MAXLINE], receivebuffer[MAXLINE];
    FILE *fp = stdin;
    fd_set rset;
    int maxfdp1;

    if(argc != 3){
        printf("wrong input\n");
        exit(1);
    }

    serverIP = argv[1];
    serverPORT = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPORT);
    inet_pton(AF_INET, serverIP, &server.sin_addr);

    FD_ZERO(&rset);

    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("connect error\n");
        exit(1);
    }

    for(;;){
        FD_SET(0, &rset);
        FD_SET(sockfd, &rset);

        if(0 > sockfd)maxfdp1 = 0 + 1;
        else maxfdp1 = sockfd + 1;

        select(maxfdp1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(0, &rset)){
            fgets(buffer, MAXLINE, fp);
            write(sockfd, buffer, sizeof(buffer));
        }

        if(FD_ISSET(sockfd, &rset)){
            ssize_t len;
            len = read(sockfd, receivebuffer, MAXLINE);
            if(len == 0){
               close(sockfd);
               exit(0);
             }
            fputs(receivebuffer, stdout);
        }
    }
}