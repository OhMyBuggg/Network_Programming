#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>

#define backlog 10
#define MAXLINE 1024

int main(int argc, char *argv[]){
	int serverPORT;
	int sockfd, listenfd, clientfd;
	char message[MAXLINE];
	struct sockaddr_in serverAddress, clientAddress;
	char *clientIP[backlog], *username[backlog];
	int maxfd, maxClient, numReady, i, j;
	socklen_t clientlen; 
	int client[backlog], clientPort[backlog], ban[backlog];//save all client's sockfd, and port number
	fd_set allset, rset; //for select
	char sendbuffer[MAXLINE], receivebuffer[MAXLINE], receiver[MAXLINE];
	char command[MAXLINE], text[MAXLINE];
	char oldname[MAXLINE];
	ssize_t len;

	serverPORT = atoi(argv[1]);

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket erroe\n");
		exit(1);
	} //listen socket
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPORT); // port is unsigned short int, set server is on port 8080
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	//every ip to this server, s_addr is 32 bytes => long

	if(bind(listenfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
		printf("bind failed\n");
		exit(1);
	} 
	// bind server infomation to listen socket

	listen(listenfd, backlog); //let listen be the socket to wait for connect, and up to backlog
	
	maxfd = listenfd; // current max fd is listen
	maxClient = -1; // no client is recorded
	for(int i=0; i<backlog; i++){
		client[i] = -1;
		ban[i] = 0;
	} // no client
	FD_ZERO(&allset); //clear allset
	FD_SET(listenfd, &allset); // put listen socket in allset

	for(;;){ //server run forever
		rset = allset; // avoid change allset 
		numReady = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){ //listen socket is readable = have client
			clientlen = sizeof(clientAddress);
			clientfd = accept(listenfd, (struct sockaddr *)&clientAddress, &clientlen); 
			// accept a new client and give it a new socket

			for(i=0; i<backlog; i++){
				if(client[i] < 0){
					client[i] = clientfd;
					break;
				}
			} // record i is what fd
			if(i == backlog){
				printf("To many client\n");
				exit(1);
			}
			//printf("client is fd %d\n", i);

			clientIP[i] = malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP[i], INET_ADDRSTRLEN);
			clientPort[i] = ntohs(clientAddress.sin_port);
			username[i] = malloc(MAXLINE * sizeof(char));
			strcpy(username[i], "anonymous"); //client information finished
			
			FD_SET(clientfd, &allset);
			if(clientfd > maxfd) maxfd = clientfd;
			if(i > maxClient) maxClient = i;

			for(j=0; j <= maxClient; j++){
				if(clientfd == client[j]){
					sprintf(message, "[Server] Hello, anonymous! From: %s:%d\n", clientIP[j], clientPort[j]);
					write(client[j], message, sizeof(message));
				}else if(client[j] >= 0){
					strcpy(message, "[Server] Someone is coming!\n");
					write(client[j], message, sizeof(message));
				}
			}
			
			numReady--;
			if(numReady <= 0) continue;
		} //connect handle finish

		for(i=0; i <= maxClient; i++){
			sockfd = client[i];
			if(client[i] < 0)continue;

			if(FD_ISSET(sockfd, &rset)){ //someone is readable
				ssize_t n, templen;
				char templetter;
				char *tempbuffer;
				tempbuffer = receivebuffer;
				for(n=1; n<MAXLINE; n++){
					if((templen = read(sockfd, &templetter, 1)) == 1){
						*tempbuffer++ = templetter;
						if(templetter == '\n') break;
					}else if(templen == 0){
						*tempbuffer = '\0';
						n--;
						break;
					}
				}
				*tempbuffer = '\0';
				len = n;
				printf("receive length %zd\n", len);

				if(len == 0){ //client closed
					printf("client port %d leave\n", clientPort[i]);

					for(j = 0; j <= maxClient; j++){
						if(client[j] == sockfd){
							continue;
						}else if(client[j] >= 0){
							sprintf(message, "[Server] %s is offline.\n", username[i]);
							write(client[j], message, sizeof(message));
						}
					}

					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					free(clientIP[i]);
					free(username[i]);

				}else{
					if(ban[i] == 1){
						sprintf(message, "you can not send message\n");
						write(client[i], message, sizeof(message));
						continue;
					}
					printf("line: %s\n", receivebuffer);
					strcpy(command, strtok(receivebuffer, " "));
					if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';
					strcpy(text, receivebuffer + strlen(command) +1 );
					if(text[strlen(text) - 1] == '\n') text[strlen(text) - 1] = '\0';

					if(strcmp(command, "who") == 0){
						printf("someone input who\n");
						for(j=0; j <= maxClient; j++){
							if(client[j] == sockfd){
								sprintf(message, "[Server] %s %s:%d ->me\n", username[j], clientIP[j], clientPort[j]);
								write(client[i], message, sizeof(message));
							}else if(client[j] >= 0){
								sprintf(message, "[Server] %s %s:%d\n", username[j], clientIP[j], clientPort[j]);
								write(client[i], message, sizeof(message));
							}
						}
					}else if(strcmp(command, "name") == 0){
						printf("someone input name\n");
						if(strcmp(text, "anonymous") == 0){
							strcpy(message, "[Server] ERROR: Username cannot be anonymous.\n");
							write(client[i], message, sizeof(message));
							continue;
						} // no anonymous
						for(j=0; j <= maxClient; j++){
							if(strcmp(text, username[j]) == 0 && client[j] != sockfd){
								sprintf(message, "[Server] ERROR: %s has been used by others.\n", text);
								write(client[i], message, sizeof(message));
								break;
							}
						}
						if(j <= maxClient) continue; // no the same

						int allenglish = 1;
						for(j=0; j < strlen(text); j++){
							if(text[j] < 'A' || text[j] > 'z'){
								allenglish = 0;
								break;
							}
						}
						if(strlen(text) < 2 || strlen(text) > 12 || allenglish == 0){
							strcpy(message, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
							write(client[i], message, sizeof(message));
							continue;
						}

						strcpy(oldname, username[i]);
						strcpy(username[i], text); //update username

						for(j=0; j <= maxClient; j++){
							if(client[j] == sockfd){
								sprintf(message, "[Server] You're now known as %s.\n", text);
								write(client[i], message, sizeof(message));
							}else if(client[j] >= 0 && ban[j] == 0){
								sprintf(message, "[Server] %s is now known as %s.\n", oldname, text);
								write(client[j], message, sizeof(message));
							}
						}
					}else if(strcmp(command, "tell") == 0){
						printf("someone input tell\n");
						if(strcmp(username[i], "anonymous") == 0){
							strcpy(message, "[Server] ERROR: You are anonymous.\n");
							write(client[i], message, sizeof(message));
							continue;
						}

						strcpy(receiver, strtok(text, " "));
						if(receiver[strlen(receiver) - 1] == '\n')receiver[strlen(receiver) - 1] = '\0'; // the user want to send

						strcpy(sendbuffer, text + strlen(receiver) + 1);
						if(sendbuffer[strlen(sendbuffer) - 1] == '\n')sendbuffer[strlen(sendbuffer) - 1] = '\0'; // the message want to send

						if(strcmp(receiver, "anonymous") == 0){
							strcpy(message, "[Server] ERROR: The client to which you sent is anonymous.\n");
							write(client[i], message, sizeof(message));
							continue;
						}

						for(j=0; j <= maxClient; j++){
							if(strcmp(receiver, username[j]) == 0){
								break;
							}
						} // user[j] is receiver

						if(j > maxClient){
							strcpy(message, "[Server] ERROR: The receiver doesn't exist.\n");
							write(client[i], message, sizeof(message));
							continue;
						}else if(ban[j] == 1){
							sprintf(message, "that user has been banned\n");
							write(client[i],message, sizeof(message));
							continue;
						}else{
							strcpy(message, "[Server] SUCCESS: Your message has been sent.\n");
							write(client[i], message, sizeof(message));
							sprintf(message, "[Server] %s tell you %s\n", username[i], sendbuffer);
							write(client[j], message, sizeof(message));
							continue;
						}
					}else if(strcmp(command, "yell") == 0){
						printf("someone input yell\n");
						for(j=0; j <= maxClient; j++){
							if(client[j] >= 0 && ban[j] == 0){
								sprintf(message, "[Server] %s yell %s\n", username[i], text);
								write(client[j], message, sizeof(message));
							}
						}
					}else if(strcmp(command, "ban") == 0){
						printf("bannnnn\n");
						for(j=0; j<= maxClient; j++){
							printf("%s\n", username[j]);
							if(strcmp(text, username[j]) == 0){
								printf("find\n");
								ban[j] = 1;
								sprintf(message, "you have ban %s\n", username[j]);
								write(client[i], message, sizeof(message));
								break;
							}
						}
						if(j > maxClient){
							sprintf(message, "wrong user\n");
							write(client[i], message, sizeof(message));
						}
					}else{
						strcpy(message, "[Server] ERROR: Error command.\n");
						write(client[i], message, sizeof(message));
					}	
				}

				numReady--;
				if(numReady <= 0)break;
			}
		}
	}

	return 0;
}