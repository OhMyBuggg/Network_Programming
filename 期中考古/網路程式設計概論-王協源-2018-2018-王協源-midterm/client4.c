#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define MAXLINE 1024
#define backlog 10

int main(int argc, char *argv[]){
	int myPORT, friendPORT;
	char *myIP, *friendIP, *myNAME;
	char sendbuffer[MAXLINE], receivebuffer[MAXLINE], receiver[MAXLINE], sendMessage[MAXLINE], message[MAXLINE];
	char *friendNAME[backlog];
	struct sockaddr_in myAddr, friendAddr;
	int mysockfd, maxfd, maxFriend, numReady, friendfd;
	int friend[backlog];
	fd_set allset, rset;
	FILE *fp = stdin;
	int i,j;

	myIP = "127.0.0.1";
	myPORT = atoi(argv[2]);
	myNAME = argv[1];
	mysockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&myAddr, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(myPORT);
	inet_pton(AF_INET, myIP, &myAddr.sin_addr);
	// my information

	if(bind(mysockfd, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0){
		printf("bind error\n");
		exit(1);
	}
	// bind my infor to socket

	listen(mysockfd, backlog);
	maxfd = mysockfd;
	maxFriend = -1;
	for(i=0; i<backlog; i++){
		friend[i] = -1;
	}

	FD_ZERO(&allset);
	FD_SET(mysockfd, &allset);
	FD_SET(fileno(fp), &allset);
	if(fileno(fp) > maxfd) maxfd = fileno(fp);

	for(i=3; i<argc; i++){
		friendPORT = atoi(argv[i]);
		friendIP = "127.0.0.1";
		//printf("try to connect port %d\n", friendPORT);
		friendfd = socket(AF_INET, SOCK_STREAM, 0);

		bzero(&friendAddr, sizeof(friendAddr));
		friendAddr.sin_family = AF_INET;
		friendAddr.sin_port = htons(friendPORT);
		inet_pton(AF_INET, friendIP, &friendAddr.sin_addr);

		if(connect(friendfd, (struct sockaddr *)&friendAddr, sizeof(friendAddr)) < 0){
			printf("Connect error\n");
			exit(1);
		}
		for(j=0; j<backlog; j++){
			if(friend[j] < 0){
				friend[j] = friendfd;
				break;
			}
		}

		//printf("connect success\n");
		write(friendfd, myNAME, sizeof(myNAME));
		read(friendfd, &receivebuffer, MAXLINE);
		//printf("port %d's name is %s\n", friendPORT, receivebuffer);
		friendNAME[j] = malloc(MAXLINE * sizeof(char));
		strcpy(friendNAME[j], receivebuffer);
		//printf("copy name done\n");
		if(friendfd > maxfd) maxfd = friendfd;
		if(j > maxFriend) maxFriend = j;
		FD_SET(friendfd, &allset);

		//printf("connection done\n");
	}
	// connect to everybody

	//printf("friend number is %d\n", maxFriend);
	//printf("friend max fd is %d\n", maxfd);

	for(;;){
		rset = allset;
		numReady = select(maxfd + 1, &rset, NULL, NULL, NULL);
		//printf("select done once\n");

		if(FD_ISSET(mysockfd, &rset)){
			//printf("someone try to connect\n");
			friendfd = accept(mysockfd, (struct sockaddr *)NULL, NULL);
			write(friendfd, myNAME, sizeof(myNAME));

			for(i=0; i<backlog; i++){
				if(friend[i] < 0){
					friend[i] = friendfd;
					break;
				}
			}
			if(i == backlog){
				printf("Too many friends\n");
				exit(1);
			}

			read(friendfd, &receivebuffer, MAXLINE);
			//printf("receive name %s\n", receivebuffer);
			friendNAME[i] = malloc(MAXLINE * sizeof(char));
			strcpy(friendNAME[i], receivebuffer);

			FD_SET(friendfd, &allset);
			if(friendfd > maxfd) maxfd = friendfd;
			if(i > maxFriend) maxFriend = i;

			//printf("friend number is %d\n", maxFriend);
			//printf("friend max fd is %d\n", maxfd);

			numReady--;
			if(numReady <= 0)continue;
		}
		//save friend info

		if(FD_ISSET(fileno(fp), &rset)){
			fgets(sendbuffer, MAXLINE, fp);
			if(strcmp(sendbuffer, "EXIT\n") == 0){
				for(i=0; i<backlog; i++){
					if(friend[i] >= 0){
						close(friend[i]);
					}
				}
				exit(0);
			}
			strcpy(receiver, strtok(sendbuffer, " "));
			if(receiver[strlen(receiver) - 1] == '\n')receiver[strlen(receiver) - 1] = '\0';
			strcpy(sendMessage, sendbuffer + strlen(receiver) + 1);
			if(sendMessage[strlen(sendMessage) - 1] == '\n')sendMessage[strlen(sendMessage) - 1] = '\0';

			if(strcmp(receiver, "ALL") == 0){
				for(i=0; i<backlog; i++){
					if(friend[i] >= 0){
						sprintf(message, "%s SAID %s\n", myNAME, sendMessage);
						write(friend[i], message, sizeof(message));
					}
				}
			}else{
				for(i=0; i<backlog; i++){
					if(friend[i] >= 0 && strcmp(friendNAME[i], receiver) == 0){
						sprintf(message, "%s SAID %s\n", myNAME, sendMessage);
						write(friend[i], message, sizeof(message));
						break;
					}
				}
				if(i >= backlog){
						sprintf(message, "No user\n");
						fputs(message, stdout);
					}
			}
		}
		//user input

		for(i=0; i<=maxFriend; i++){
			//printf("some has thing to say\n");
			if(friend[i] < 0) continue;
			if(FD_ISSET(friend[i], &rset)){
				ssize_t len;
				len = read(friend[i], receivebuffer, MAXLINE);
				if(len == 0){
					printf("%s has left the chat room\n", friendNAME[i]);
					close(friend[i]);
					FD_CLR(friend[i], &allset);
					friend[i] = -1;
					free(friendNAME[i]);
					continue;
				}
				fputs(receivebuffer, stdout);
				numReady--;
				if(numReady <= 0)break;
			}
		}

	}
}