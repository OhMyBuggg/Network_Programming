#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024

int main(int argc, char *argv[]){
	char *serverIP;
	int serverPORT;
	int sockfd;
	struct sockaddr_in server;
	char sendbuffer[MAXLINE], receivebuffer[MAXLINE];
	int n;
	FILE *fp = stdin;

	serverIP = argv[1];
	serverPORT = atoi(argv[2]);

	if(argc != 3){
		printf("Wrong input\n");
		exit(1);
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket error\n");
		exit(1);
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	if((inet_pton(AF_INET, serverIP, &server.sin_addr)) <= 0){
		printf("inet_pton error\n");
		exit(1);
	}

	if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
		printf("connect error\n");
		exit(1);
	}

	for(;;){
		fgets(sendbuffer, MAXLINE, fp);
		write(sockfd, sendbuffer, sizeof(sendbuffer));

		ssize_t len;
		len = read(sockfd, receivebuffer, MAXLINE);
		if(len == 0){
			printf("connection closed\n");
			exit(1);
		}
		fputs(receivebuffer, stdout);
	}
	exit(0);
}