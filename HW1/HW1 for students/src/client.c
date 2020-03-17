#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h> //socket,connect
#include <netinet/in.h> // hton
#include <arpa/inet.h> //inet_pton
#include <sys/select.h>
#include <unistd.h> //read, write
#include <sys/types.h>

#define MAXLINE 1024

int main(int argc, char *argv[]){
	char *serverIP;
	int serverPORT;
	int sockfd, maxfdp1;
	struct sockaddr_in serverAddress;
	fd_set rset;
	char sendbuffer[MAXLINE], receivebuffer[MAXLINE];
	FILE *fp = stdin;
	int stdineof;

	if(argc != 3){
		printf("Incorrect input\n");
		exit(1);
	}

	serverIP = argv[1];
	serverPORT = atoi(argv[2]);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Socket error\n");
		exit(1);
	} //build socket

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET; //ipv4
	serverAddress.sin_port = htons(serverPORT); //sin_port is network short int 
	if((inet_pton(AF_INET, serverIP, &serverAddress.sin_addr)) <= 0){
		printf("inet_pton error\n");
		exit(1);
	} 
	//sin_addr is network type, inet_pton return in_addr type

	if((connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0){
		printf("Connect error\n");
		exit(1);
	}
	// connet to server, need to be careful of sockaddr and sockaddr_in type
	FD_ZERO(&rset);
	stdineof = 0;
  
	for(;;){
		FD_SET(0, &rset);
		FD_SET(sockfd, &rset); //set two fd_set

		if(0 > sockfd) maxfdp1 = 0 + 1;
		else maxfdp1 = sockfd + 1; //update maxfd

		select(maxfdp1, &rset, NULL, NULL, NULL); // listen socket and stdin is readable or not

		if(FD_ISSET(0, &rset)){
			fgets(sendbuffer, MAXLINE, fp);
			if(strcmp(sendbuffer, "exit\n") == 0){
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
				FD_CLR(0, &rset);
				continue;
			}//exit
			write(sockfd, sendbuffer, strlen(sendbuffer));
		}//something input

		if(FD_ISSET(sockfd, &rset)){
			ssize_t len;
			len = read(sockfd, receivebuffer, MAXLINE);
			if(len == 0){
				if(stdineof == 1){
					exit(0);
				}else{
					printf("Server terminate\n");
					exit(1);
				}
			}
			fputs(receivebuffer, stdout);
		}//server send something
	}	
	return 0;
}