#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAXLINE 1024
#define backlog 10

struct package {
	int len;
	int flag;
	char filename[MAXLINE];
	char data[MAXLINE];
};

struct Client {
	int fd;
	char username[MAXLINE];
	int filesize;
	int current_size;
	FILE *fp;
} client[backlog];

void download(char *filename, int filesize, int i) {
	struct package sendp;
	int temp = 0;
	int unit;
	char path[MAXLINE];
	sprintf(path, "./%s/%s", client[i].username, filename);
	FILE *fp = fopen(path, "rb");

	sendp.len = filesize;
	sendp.flag = 0;
	strcpy(sendp.filename, filename);
	write(client[i].fd, &sendp, sizeof(sendp));

	unit = filesize/22;

	while(sendp.len = fread(sendp.data, 1, MAXLINE, fp)) {
		sendp.flag = 1;
		write(client[i].fd, &sendp, sizeof(sendp));
		temp += sendp.len;
		if(temp > unit) {
			//usleep(100000);
			temp -= unit;
		}
		usleep(4000);
	}
	fclose(fp);
}

void sendall(char *filename, int i) {
	char name[MAXLINE], path[MAXLINE];
	int filesize = client[i].filesize;
	strcpy(name, client[i].username);
	sprintf(path, "./%s/%s", client[i].username, filename);

	client[i].filesize = 0;
	client[i].current_size = 0;
	FILE *fp = fopen(path, "rb");
	for(int j = 0; j < backlog; j++) {
		if(client[j].fd >= 0 && j != i && (strcmp(client[j].username, client[i].username) == 0)) {
			download(filename, filesize, j);
		}
	}
}

void handle(int i) {
	struct package recvp, sendp;
	char buf[MAXLINE];
	int len;

	len = read(client[i].fd, &recvp, sizeof(recvp));
	if(len < 0) {
		if(errno != EWOULDBLOCK) {
			printf("read error on socket %d\n", client[i].fd);
			exit(1);
		}
	} else if (len == 0) {
		close(client[i].fd);
		client[i].fd = -1;
		client[i].filesize = -1;
		client[i].current_size = 0;
		client[i].fp = NULL;
		strcpy(client[i].username, "");
	} else {
		if(recvp.flag == 0) {
			char path[MAXLINE];
			sprintf(path, "./%s/%s", client[i].username, recvp.filename);

			client[i].filesize = recvp.len;
			client[i].fp = fopen(path, "wb");
		} else if(recvp.flag == 1) {
			fwrite(recvp.data, 1, recvp.len, client[i].fp);
			client[i].current_size += recvp.len;
			if(client[i].current_size == client[i].filesize) {
				sendp.flag = 2;
				sprintf(sendp.data, "[Upload] %s Finish!", recvp.filename);
				write(client[i].fd, &sendp, sizeof(sendp));
				fclose(client[i].fp);

				sendall(recvp.filename, i);
			}
		}
	}
}

void server_func(int listenfd) {
	int clientfd;

	listen(listenfd, backlog);
	for(int i = 0; i < backlog; i++){
		client[i].fd = -1;
		client[i].filesize = -1;
		client[i].current_size = 0;
		strcpy(client[i].username, "");
	}

	for(;;) {
		//build connection
		if((clientfd = accept(listenfd, NULL, NULL)) > 0) {
			int i;
			char name[MAXLINE];
			for(i = 0; i < backlog; i++) {
				if(client[i].fd < 0) {
					client[i].fd = clientfd;
					break;
				}
			}
			int num;
			num = read(client[i].fd, name, MAXLINE);
			//printf("%s is connect\n", name);
			sprintf(client[i].username, "%s", name);

			int flag;
			flag = fcntl(client[i].fd, F_GETFL, 0);
			fcntl(client[i].fd, F_SETFL, flag | O_NONBLOCK);
			int size = 100000;
			//setsockopt(client[i].fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));

			DIR *dp = opendir(name);
			struct dirent *file;
			if(dp) {
				while((file = readdir(dp)) != NULL) {
					if(strncmp(file->d_name, ".", 1) == 0)
						continue;
					char path[MAXLINE];
					sprintf(path, "./%s/%s", name, file->d_name);
					FILE *fp = fopen(path, "rb");
					fseek(fp, 0, SEEK_END);
					int filesize = ftell(fp);
					fclose(fp);
					download(file->d_name, filesize, i);
				}
				closedir(dp);
			} else {
				mkdir(name, S_IRWXU | S_IRWXG | S_IRWXO);
			}
		}

		for(int i = 0; i < backlog; i++) {
			if(client[i].fd >= 0)
				handle(i);
		}
	}
}

int main(int argc, char *argv[]) {
	int serverPORT;
	int listenfd;
	struct sockaddr_in server;
	int flag;

	if(argc != 2){
		printf("usage: ./server <port>\n");
		exit(1);
	}

	serverPORT = atoi(argv[1]);
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error\n");
		exit(1);
	}
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	flag = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flag | O_NONBLOCK);

	if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("bind failed\n");
		exit(1);
	}

	server_func(listenfd);

	return 0;
}