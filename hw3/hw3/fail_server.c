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

#define MAXLINE 20
#define backlog 10
#define MAXFILE 100

struct fileInfo {
	char username[MAXLINE];
	char filename[MAXLINE];
	int filesize;
};

struct standby {
	char *path;
	char *filename;
	int filesize;
};

struct clientInfo {
	int fd;
	char *id;
	FILE *fp;
	FILE *fp_write;
	char *filename;
	int filesize;
	char to[MAXLINE], fr[MAXLINE];
	char *toiptr, *tooptr, *friptr, *froptr;
	int downloadinfo;
	int file_standby;
	struct standby st_file[MAXFILE];
};

void server_func(int listenfd) {
	int maxfd, maxClient;
	struct clientInfo client[backlog];
	struct fileInfo file[MAXFILE];
	fd_set rset, wset;
	int numReady;
	struct sockaddr_in clientAddress;
	socklen_t clientlen;
	int clientfd;
	char *username[backlog];
	char recvnamebuf[MAXLINE];
	int i;
	ssize_t n, nwritten;
	int file_count = 0;


	listen(listenfd, backlog);
	maxfd = listenfd;
	maxClient = -1;
	for(i = 0; i < backlog; i++) {
		client[i].fd = -1;
		client[i].toiptr = client[i].tooptr = client[i].to;
		client[i].friptr = client[i].froptr = client[i].fr;
		client[i].downloadinfo = 0;
		client[i].file_standby = 0;
	}

	for(;;) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listenfd, &rset);

		for(i = 0; i < backlog; i++) {
			if(client[i].fd < 0) continue;

			if(client[i].file_standby > 0 && client[i].fp_write == NULL) {
				client[i].file_standby--;
				int now = client[i].file_standby; 
				client[i].fp_write = fopen(client[i].st_file[now].path, "rb");
				client[i].filename = client[i].st_file[now].filename;
				client[i].filesize = client[i].st_file[now].filesize;
			} else if (client[i].fp_write != NULL) {
				if(client[i].downloadinfo == 0) {
					char info[MAXLINE];
					sprintf(info, "download %s %d", client[i].filename, client[i].filesize);
					memcpy(client[i].toiptr, info, strlen(info));
					client[i].toiptr += strlen(info);
					client[i].downloadinfo = 1;
				} else {
					size_t num;
					num = fread(client[i].toiptr, 1, &(client[i].to[MAXLINE]) - client[i].toiptr, client[i].fp_write);
					client[i].toiptr += num;
					if(feof(client[i].fp_write)) {
						char ack[MAXLINE];
						strcpy(ack, "done");
						write(client[i].fd, ack, strlen(ack));
						client[i].downloadinfo = 0;
						fclose(client[i].fp_write);
					}
				}
			}

			if(client[i].friptr < &(client[i].fr[MAXLINE]))
				FD_SET(client[i].fd, &rset);
			if(client[i].tooptr != client[i].toiptr)
				FD_SET(client[i].fd, &wset);
		}

		numReady = select(maxfd + 1, &rset, &wset, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)) {
			printf("someone is in\n");
			clientlen = sizeof(clientAddress);
			clientfd = accept(listenfd, (struct sockaddr *)&clientAddress, &clientlen);
			printf("accept someone\n");

			for(i = 0; i < backlog; i++) {
				if(client[i].fd < 0) {
					client[i].fd = clientfd;
					break;
				}
			}
			if(i == backlog) {
				printf("too many client\n");
				exit(1);
			}

			ssize_t len;
			len = read(client[i].fd, recvnamebuf, MAXLINE);
			client[i].id = (char *)malloc(sizeof(char) * len);
			memcpy(client[i].id, recvnamebuf, len);

			mkdir(client[i].id, S_IRWXU | S_IRWXG | S_IRWXO);

			int flag;
			flag = fcntl(client[i].fd, F_GETFL, 0);
			fcntl(client[i].fd, F_SETFL, flag | O_NONBLOCK);

			if(clientfd > maxfd)maxfd = clientfd;
			if(i > maxClient)maxClient = i;
			FD_SET(clientfd, &rset);

			for(int j = 0; j < file_count; j++) {
				if(file[j].username == client[i].id) {
					char sendinfo[MAXLINE];
					char path[MAXLINE];
					FILE *fp_temp;
					sprintf(sendinfo, "download %s %d", file[j].filename, file[j].filesize);
					sprintf(path, "./%s/%s", file[j].username, file[j].filename);

					write(client[i].fd, sendinfo, strlen(sendinfo));
					fp_temp = fopen(path, "rb");
					while(!feof(fp_temp)) {
						char data[MAXLINE];
						size_t = num;
						num = fread(data, 1, MAXLINE, fp_temp);
						write(client[i].fd, data, num-1);
					}
					strcpy(sendinfo, "done\n");
					usleep(100000);
					write(client[i].fd, sendinfo, strlen(sendinfo)-1);
					//要解決如何non-blocking下載之前的所有檔案
				}
			}

			numReady--;
			if(numReady <= 0)continue;
		}// connection

		for(i = 0; i <= maxClient; i++) {
			if(client[i].fd < 0) continue;

			if(FD_ISSET(client[i].fd, &rset)) {
				if((n = read(client[i].fd, client[i].friptr, &(client[i].fr[MAXLINE]) - client[i].friptr)) < 0) {
					if(errno != EWOULDBLOCK)
						printf("read error on socket: %d\n", client[i].fd);
				} else if(n == 0) {
					printf("socket %d closed\n", client[i].fd);
					close(client[i].fd);
					client[i].fd = -1;
					client[i].toiptr = client[i].tooptr = client[i].to;
					client[i].friptr = client[i].froptr = client[i].fr;
					free(client[i].id);
				} else {
					printf("read %ld words from client\n", n);
					char buf[MAXLINE];
					strncpy(buf, client[i].fr, 4);
					printf("buf data is %s\n", buf);
					if(strcmp(buf, "done") == 0) { //兩個人同時傳完, fp_write會被吃掉
						fwrite("\n", 1, 1, client[i].fp);
						fclose(client[i].fp);
						printf("close client: %d's fp\n", client[i].fd);
						char path[MAXLINE];
						sprintf(path, "./%s/%s", client[i].id, file[file_count].filename);
						for(int j = 0; j < backlog; j++) {
							if(j != i && client[j].id == client[i].id) {
								int now = client[j].file_standby;
								client[j].st_file[now].path = path;
								client[j].st_file[now].filename = file[file_count].filename;
								client[j].st_file[now].filesize = file[file_count].filesize;
								client[j].file_standby++;
							}
						}
						file_count++;
						continue;
					}
					char command[MAXLINE];
					strcpy(command, strtok(client[i].friptr, " "));
					if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';
					if(strcmp(command, "put") == 0) {
						char temp_file_name[MAXLINE];
						char temp_file_size[MAXLINE];
						
						strcpy(temp_file_name, client[i].friptr + strlen(command) + 1);
						if(temp_file_name[strlen(temp_file_name) - 1] == '\n') temp_file_name[strlen(temp_file_name) - 1] = '\0';

						strcpy(file[file_count].filename, strtok(temp_file_name, " "));
						if(file[file_count].filename[strlen(file[file_count].filename) - 1] == '\n') file[file_count].filename[strlen(file[file_count].filename) - 1] = '\0';
						
						strcpy(temp_file_size, client[i].friptr + strlen(command) + strlen(file[file_count].filename) + 1);
						if(temp_file_size[strlen(temp_file_size) - 1] == '\n') temp_file_size[strlen(temp_file_size) - 1] = '\0';

						file[file_count].filesize = atoi(temp_file_size);
						strcpy(file[file_count].username, client[i].id);

						char path[MAXLINE];
						sprintf(path, "./%s/%s", client[i].id, file[file_count].filename);
						printf("path is %s\n", path);
						client[i].fp = fopen(path, "wb");
						printf("recv filename: %s, filesize: %d\n", file[file_count].filename, file[file_count].filesize);
					} else {
						printf("recv %ld bytes from %d: %s\n", n, client[i].fd, client[i].friptr);
						fwrite(client[i].friptr, 1, n, client[i].fp);
					}
				}
			}

			if(FD_ISSET(client[i].fd, &wset) && ((n = client[i].toiptr - client[i].tooptr) > 0)) {
				if((nwritten = write(client[i].fd, client[i].tooptr, n)) < 0 ) {
					if(errno != EWOULDBLOCK)
						printf("write error on socket: %d\n", client[i].fd);
				} else {
					printf("write %ld bytes to socket %d\n", nwritten, client[i].fd);
					client[i].tooptr += nwritten;
					if(client[i].tooptr == client[i].toiptr) {
						client[i].tooptr = client[i].toiptr = client[i].to;
					}
				}
			}

			numReady--;
			if(numReady <= 0) break;
		}//see which client can read or write

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

	if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("bind failed\n");
		exit(1);
	}

	flag = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flag | O_NONBLOCK);

	server_func(listenfd);

	return 0;
}