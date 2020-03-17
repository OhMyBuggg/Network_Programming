#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>

#define MAXLINE 1024

int pid;
struct package {
	int len;
	int flag;
	char filename[MAXLINE];
	char data[MAXLINE];
};

void from_terminal(int sockfd, char *command) {
	if(strncmp(command, "exit", 4) == 0) {
		close(sockfd);
		exit(0);
	} else if(strncmp(command, "put", 3) == 0) {
		char f_name[MAXLINE];
		struct package sendp;
		int filesize, current_size = 0;
		int hashtag = 0;
		int unit, temp = 0;
		int c = 0;

		strcpy(f_name, command + 4);
		if(f_name[strlen(f_name) - 1] == '\n') f_name[strlen(f_name) - 1] = '\0';
		FILE *fp = fopen(f_name, "rb");

		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		rewind(fp);

		sendp.len = filesize;
		sendp.flag = 0;
		strcpy(sendp.filename, f_name);
		write(sockfd, &sendp, sizeof(sendp));

		unit = filesize/22;

		printf("Pid: %d [Upload] %s Start!\n", pid, f_name);
		printf("Pid: %d Progress : [%s]\r", pid, "                      ");
		fflush(stdout);
		while(1) {
			// int size;
			// ioctl( sockfd, TIOCOUTQ, &size );
			// if(740000 - size < 30000){
			// 	//printf("stop a time\n");
			// 	continue;
			// }

			sendp.len = fread(sendp.data, 1, MAXLINE, fp);
			if(sendp.len <= 0) {
				break;
			}
			sendp.flag = 1;
			write(sockfd, &sendp, sizeof(sendp)); //20488
			current_size += sendp.len;
			temp += sendp.len;
			if(current_size == filesize) {
				break;
			}

			// int size;
			// ioctl( sockfd, TIOCOUTQ, &size );
			// printf("remain: %d\n", size);
			// if(size > 100000) {
			// 	usleep(10000);
			// }

			char progressbar[22];
			if(temp > unit) {
				//usleep(100000);
				hashtag++;
				temp -= unit;
				for(int i = 0; i < 22; i++) {
					if(i < hashtag) progressbar[i] = '#';
					else progressbar[i] = ' ';
				}
				printf("Pid: %d Progress : [%s]\r", pid, progressbar);
				fflush(stdout);
			}
			usleep(4000);
		}
		printf("Pid: %d Progress : [%s]\n", pid, "######################");
	} else if(strncmp(command, "sleep", 5) == 0) {
		char time[MAXLINE];
		strcpy(time, command + 6);
		if(time[strlen(time) - 1] == '\n') time[strlen(time) - 1] = '\0';
		int t = atoi(time);
		printf("Pid: %d The client starts to sleep.\n", pid);
		for(int i = 1; i <= t; i++) {
			usleep(990000);
			printf("Pid: %d Sleep %d\n", pid, i);
		}
		printf("Pid: %d Client wakes up.\n", pid);
	} else {
		printf("wrong command\n");
	}
}

void client(int sockfd) {
	struct package recvp;
	char command[MAXLINE], filename[MAXLINE];
	int len, filesize, current_size = 0;;
	int unit, temp = 0;
	int hashtag = 0;
	int flag;
	FILE *fp;

	flag = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flag | O_NONBLOCK);

	flag = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
	int size = 100000;
	//setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));

	for(;;) {
		if((len = read(0, command, sizeof(command))) > 0) {
			from_terminal(sockfd, command);
		}
		if((len = read(sockfd, &recvp, sizeof(recvp))) >= 0) {
			if(len == 0) {
				printf("server terminate\n");
				exit(1);
			}
			if(recvp.flag == 0) {
				fp = fopen(recvp.filename, "wb");
				filesize = recvp.len;
				unit = filesize/22;
				strcpy(filename, recvp.filename);
				printf("Pid: %d [Download] %s Start!\n", pid, filename);
				printf("Pid: %d Progress : [%s]\r", pid, "                      ");
				fflush(stdout);
			} else if (recvp.flag == 1) {
				fwrite(recvp.data, 1, recvp.len, fp);

				temp += recvp.len;
				current_size += recvp.len;
				if(current_size == filesize) {
					printf("Pid: %d Progress : [%s]\n", pid, "######################");
					fclose(fp);
					hashtag = temp = current_size = 0;
					printf("Pid: %d [Download] %s Finish!\n", pid, recvp.filename);
					continue;
				}

				char progressbar[22];
				if(temp > unit) {
					hashtag++;
					temp -= unit;
					for(int i = 0; i < 22; i++) {
						if(i < hashtag) progressbar[i] = '#';
						else progressbar[i] = ' ';
					}
					printf("Pid: %d Progress : [%s]\r", pid, progressbar);
					fflush(stdout);
				}
			} else if(recvp.flag == 2) {
				printf("Pid: %d %s\n", pid, recvp.data);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	char *serverIP, username[MAXLINE];
	int serverPORT;
	int sockfd;
	struct sockaddr_in serverAddress;

	if(argc != 4){
		printf("usage: ./client <ip> <port> <username>\n");
		exit(1);
	}

	pid = getpid();
	serverIP = argv[1];
	serverPORT = atoi(argv[2]);
	sprintf(username, "%s", argv[3]);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error\n");
		exit(1);
	}

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPORT);
	inet_pton(AF_INET, serverIP, &serverAddress.sin_addr);

	if((connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0) {
		printf("connect error12\n");
		exit(1);
	} else {
		printf("Pid: %d Welcome to the dropbox-like server: %s\n", pid, username);
		write(sockfd, username, MAXLINE);
	}

	client(sockfd);

	return 0;
}
