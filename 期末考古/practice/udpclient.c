#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


#define MAXLINE 1024

void echo_cli(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen) {
	char msg[MAXLINE];
	fd_set rset;
	while(1) {
		FD_ZERO(&rset);
		FD_SET(0, &rset);
		int maxfdp1 = 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(0, &rset)){
			int n = read(STDIN_FILENO, msg, MAXLINE);
			if(strcmp(msg, "-1") == 0) {
				exit(1);
			}
			msg[n-1] = '\0';
			sendto(sockfd, msg, n, 0, (struct sockaddr *)recv_addr, sizeof(*recv_addr));
			recvfrom(sockfd, msg, MAXLINE, 0, (struct sockaddr *)recv_addr, sizeof(*recv_addr));
			printf("echo msg is: %s\n", msg);
		}
	}
}

int main(int argc, char *argv[]) {
	char serverIP[INET_ADDRSTRLEN];
	int serverPORT;
	int sockfd;
	struct sockaddr_in server;
	struct addrinfo hints, *serverinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_INET;
	if(getaddrinfo(argv[1], NULL, &hints, &serverinfo) != 0) {
		printf("no such that server\n");
		exit(1);
	}

	strcpy(serverIP, inet_ntoa(((struct sockaddr_in *)serverinfo->ai_addr)->sin_addr));
	serverPORT = atoi(argv[2]);

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	inet_pton(AF_INET, serverIP, &server.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	echo_cli(sockfd, &server, sizeof(server));

	return 0;
}