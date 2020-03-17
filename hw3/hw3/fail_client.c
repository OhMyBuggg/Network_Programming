#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>

#define MAXLINE 1024

void client(int sockfd, char *username) {
	int maxfdp1;
	fd_set rset;
	FD_ZERO(&rset);
	char sendbuffer[MAXLINE], message[MAXLINE], recvbuf[MAXLINE];
	FILE *fp = stdin;
	FILE *fp_write;
	char filename[MAXLINE];
	int filesize;


	for(;;) {
		FD_SET(0, &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = sockfd + 1;

		select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(0, &rset)) {
			fgets(sendbuffer, MAXLINE, fp);
			if(strcmp(sendbuffer, "exit\n") == 0) {
				shutdown(sockfd, SHUT_WR);
				FD_CLR(0, &rset);
				exit(0);
			} else {
				char command[MAXLINE];
				char text[MAXLINE];
				strcpy(command, strtok(sendbuffer, " "));
				if(command[strlen(command) - 1] == '\n')command[strlen(command) - 1] = '\0';
				strcpy(text, sendbuffer + strlen(command) + 1);
				if(text[strlen(text) - 1] == '\n') text[strlen(text) - 1] = '\0';

				if(strcmp(command, "put") == 0) {
					FILE *fp = fopen(text, "rb");
					fseek(fp, 0, SEEK_END);
					filesize = ftell(fp);
					filesize -= 1;
					rewind(fp);
					sprintf(message, "put %s %d", text, filesize);
					printf("[Upload] %s Start!\n", text);
					write(sockfd, message, strlen(message));
					printf("send %s to server\n", message);
					while(!feof(fp)) {
						char data[MAXLINE];
						size_t num;
						num = fread(data, 1, MAXLINE, fp);
						write(sockfd, data, num-1);
					}
					strcpy(message, "done\n");
					printf("[Upload] %s Finish!\n", text);
					usleep(100000);
					write(sockfd, message, strlen(message)-1);
				} else if (strcmp(command, "sleep") == 0) {
					printf("The client starts to sleep.\n");
					int t = atoi(text);
					for(int i = 1; i <= t; i++) {
						sleep(1);
						printf("Sleep %d\n", i);
					}
					printf("Client wakes up.\n");
				} else {
					printf("wrong command\n");
					continue;
				}
			}
		}

		if(FD_ISSET(sockfd, &rset)) {
			ssize_t n;
			n = read(sockfd, recvbuf, MAXLINE);
			if(n == 0) {
				printf("server terminate\n");
				exit(1);
			} else {
				char command[MAXLINE];
				strcmp(command, strtok(recvbuf, " "));
				if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';
				if(strcmp(command, "download") == 0) {
					char temp_file_name[MAXLINE];
					char temp_file_size[MAXLINE];

					strcpy(temp_file_name, recvbuf + strlen(command) + 1);
					if(temp_file_name[strlen(temp_file_name) - 1] == '\n') temp_file_name[strlen(temp_file_name) - 1] = '\0';
					strcpy(filename, strtok(temp_file_name, " "));
					if(filename[strlen(filename) - 1] == '\n') filename[strlen(filename) - 1] = '\0';
					strcpy(temp_file_size, recvbuf + strlen(command) + strlen(filename) + 1);
					if(temp_file_size[strlen(temp_file_size) - 1] == '\n') temp_file_size[strlen(temp_file_size) - 1] = '\0';
					filesize = atoi(temp_file_size);

					fp_write = fopen(filename, "wb");
					printf("[Download] %s Start!\n", filename);
				} else if(strcmp(command, "done") == 0) {
					printf("[Download] %s Finish!\n", filename);
					fclose(fp_write);
				} else {
					fwrite(recvbuf, 1, n, fp_write);
				}
			}
		}


	}
}

int main(int argc, char *argv[]) {
	char *serverIP, *username;
	int serverPORT;
	int sockfd;
	struct sockaddr_in serverAddress;

	if(argc != 4){
		printf("usage: ./client <ip> <port> <username>\n");
		exit(1);
	}

	serverIP = argv[1];
	serverPORT = atoi(argv[2]);
	username = argv[3];

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error\n");
		exit(1);
	}

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPORT);
	inet_pton(AF_INET, serverIP, &serverAddress.sin_addr);

	if((connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0) {
		printf("connect error\n");
		exit(1);
	} else {
		printf("Welcome to the dropbox-like server: %s\n", username);
		write(sockfd, username, strlen(username));
	}

	client(sockfd, username);

	return 0;
}
