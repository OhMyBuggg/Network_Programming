#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>

#define MAXLINE 1024

int main(int argc, char *argv[]){
	int listenfd, connfd;
	struct sockaddr_in server;
	char buffer[MAXLINE];
	char cal[MAXLINE];
	int arg[10];
	unsigned long int ans;
	int serverPORT;
	char send[MAXLINE];
	int client[10];
	int maxfd, maxClient;
	fd_set allset, rset;
	int numReady;
	int c;

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

	listen(listenfd, 10);

	maxfd = listenfd;
	maxClient = -1;

	for(c=0; c<10; c++){
		client[c] = -1;
	}

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for(;;){
		rset = allset;
		numReady = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
			connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

			for(c=0; c<10; c++){
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

		for(c=0; c <= maxClient; c++){
			if(client[c] < 0)continue;

			if(FD_ISSET(client[c], &rset)){
				ssize_t len;
				len = read(client[c], &buffer, MAXLINE);

				if(len == 0){
					close(client[c]);
					FD_CLR(client[c], &allset);
					client[c] = -1;
				}else{
					if(buffer[strlen(buffer) - 1] == '\n'){
					buffer[strlen(buffer) - 1] = '\0';
					}
					//printf("read done\n");
					char *ptr = strtok(buffer, " ");
					int i = 0;
					strcpy(cal, ptr);
					//printf("%s\n", cal);

					while(1){
						char temp[MAXLINE];
						ptr = strtok(NULL, " ");
						if(ptr == NULL)break;
						strcpy(temp, ptr);
						//printf("%s\n", temp);
						arg[i] = atoi(temp);
						//printf("%d\n", arg[i]);
						i++;
					}
					if(strcmp(cal, "ADD") == 0){
						ans = 0;
						for(int j=0; j<i; j++){
							ans += arg[j];
						}
						if(ans > UINT_MAX){
							strcpy(send, "Overflowed\n");
							write(client[c], send, sizeof(send));
							continue;
						}
						sprintf(send, "%lu\n", ans); 
						write(client[c], send, sizeof(send));
					}else if(strcmp(cal, "MUL") == 0){
						ans = 1;
						for(int j=0; j<i; j++){
							ans = ans * arg[j];
						}
						if(ans > UINT_MAX){
							strcpy(send, "Overflowed\n");
							write(client[c], send, sizeof(send));
							continue;
						}
						sprintf(send, "%lu\n", ans);
						write(client[c], send, sizeof(send));
					}else if(strcmp(cal, "EXIT") == 0){
						close(client[c]);
						FD_CLR(client[c], &allset);
						client[c] = -1;
						break;
					}else{
						sprintf(send, "wrong command\n");
						write(client[c], send, sizeof(send));
					}
				}
				numReady--;
				if(numReady <= 0)break;
			}
		}
	}
}
