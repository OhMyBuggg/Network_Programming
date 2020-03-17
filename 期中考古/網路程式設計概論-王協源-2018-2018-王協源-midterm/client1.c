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
	char buffer[MAXLINE], receivebuffer[MAXLINE];
	FILE *fp = stdin;

	if(argc != 3){
		printf("wrong input\n");
		exit(1);
	}

	serverIP = argv[1];
	serverPORT = atoi(argv[2]);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket error\n");
		exit(1);
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	inet_pton(AF_INET, serverIP, &server.sin_addr);

	if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
		printf("connect error\n");
		exit(1);
	}

	for(;;){
		fgets(buffer, MAXLINE, fp);
		write(sockfd, buffer, sizeof(buffer));

		ssize_t len;
		len = read(sockfd, receivebuffer, MAXLINE);
		if(len == 0){
			printf("The server has closed the connection\n");
			close(sockfd);
			exit(0);
		}
		fputs(receivebuffer, stdout);
	}
}