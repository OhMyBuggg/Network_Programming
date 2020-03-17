#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>

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

	listen(listenfd, 1);

	for(;;){
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		for(;;){
			ssize_t len;
			len = read(connfd, &buffer, MAXLINE);
			if(len == 0){
				break;
			}

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

			//printf("%s\n", cal);
			//for(int k=0; k<i; k++){
			//	printf("%d ",arg[k]);
			//}

			if(strcmp(cal, "ADD") == 0){
				ans = 0;
				for(int j=0; j<i; j++){
					ans += arg[j];
				}
				if(ans > UINT_MAX){
					strcpy(send, "Overflowed\n");
					write(connfd, send, sizeof(send));
					continue;
				}
				sprintf(send, "%lu\n", ans); 
				write(connfd, send, sizeof(send));
			}else if(strcmp(cal, "MUL") == 0){
				ans = 1;
				for(int j=0; j<i; j++){
					ans = ans * arg[j];
				}
				if(ans > UINT_MAX){
					strcpy(send, "Overflowed\n");
					write(connfd, send, sizeof(send));
					continue;
				}
				sprintf(send, "%lu\n", ans);
				write(connfd, send, sizeof(send));
			}else if(strcmp(cal, "EXIT") == 0){
				close(connfd);
				break;
			}else{
				sprintf(send, "wrong command\n");
				write(connfd, send, sizeof(send));
			}

		}
	}
}
