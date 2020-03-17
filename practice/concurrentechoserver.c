#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define backlog 10
#define MAXLINE 1024

void sig_fork(int signo){
	pid_t pid;
	int status;
	while((pid = waitpid(-1, &status, WNOHANG)) > 0){
		printf("child %d terminated\n", pid);
	}
	return;
}

int main(int argc, char *argv[]){
	int serverPORT;
	int listenfd, connfd;
	char buffer[MAXLINE];
	struct sockaddr_in server;
	pid_t pid;

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
			for(;;){
				read(connfd, &buffer, MAXLINE);
				if(strcmp(buffer, "exit\n") == 0){
					break;
				}
				write(connfd, buffer, sizeof(buffer));
			}
			close(connfd);
			exit(0);
		}else{
			close(connfd);
		}
	}
}