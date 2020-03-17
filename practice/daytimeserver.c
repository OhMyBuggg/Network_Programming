#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define backlog 10
#define MAXLINE 1024

int main(int agrc, char *argv[]){
	int listenfd, connfd;
	struct sockaddr_in server;
	char buffer[MAXLINE];
	int serverPORT;

	serverPORT = atoi(argv[1]);

	time_t ticks;
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket error\n");
		exit(1);
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0){
		printf("bind failed\n");
		exit(1);
	}

	listen(listenfd, backlog);

	for(;;){
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		ticks = time(NULL);
		sprintf(buffer, "%.24s\n", ctime(&ticks));
		write(connfd, buffer, sizeof(buffer));
		close(connfd);
	}
}