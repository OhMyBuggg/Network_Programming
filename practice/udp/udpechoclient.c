#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#define SEG_SIZE 1024
#define INT_SIZE 32
#define SEND_TIMEO 250000

char *numtobit(uint32_t num){
	char *bit = (char *)malloc(INT_SIZE * sizeof(char));
	memset(bit, 0, INT_SIZE);

	int i, offset = 31;
	for(i = 0; i < 32; i++){
		uint32_t temp = (num & (1 << offset)) >> offset;
		bit[i] |= temp;
		offset--;
	}

	return bit;
}

uint32_t bittonum(char *bit){
	uint32_t num = 0;

	int i, offset = 31;
	for(i = 0; i < 32; i++){
		uint32_t temp = (bit[i] & (1 << offset)) >> offset;
		num |= temp;
		offset--;
	}

	return num;
}

struct msghdr* init_msghdr(struct sockaddr_in *sock_addr, socklen_t addrlen){
	struct msghdr *msg = (struct msghdr *)malloc(sizeof(struct msghdr));
	memset(msg, 0, sizeof(struct msghdr));

	struct iovec *iov = (struct ivvec *)malloc(4 * sizeof(struct iovec));

	char *type = (char *)malloc(sizeof(char));
	char *str_sn = (char *)malloc(INT_SIZE * sizeof(char));
	char *data = (char *)malloc(SEG_SIZE * sizeof(char));
	char *data_bytes = (char *)malloc(INT_SIZE * sizeof(char));
	memset(type, 0, 1);
	memset(str_sn, 0, INT_SIZE);
	memset(data, 0, SEG_SIZE);
	memset(data_bytes, 0, INT_SIZE);

	iov[0].iov_base = type;
	iov[0].iov_len = 1;
	iov[1].iov_base = str_sn;
	iov[1].iov_len = INT_SIZE;
	iov[2].iov_base = data;
	iov[2].iov_len = SEG_SIZE;
	iov[3].iov_base = data_bytes;
	iov[3].iov_len = INT_SIZE;

	msg->msg_iov = iov;
	msg->msg_iovlen = 4;
	msg->msg_name = sock_addr;
	msg->msg_namelen = addrlen;

	return msg;
}

void set_msghdr(struct msghdr *msg, char type, uint32_t send_num, char *data, uint32_t data_bytes){
	memset(msg->msg_iov[0].iov_base, 0, 1);
	memset(msg->msg_iov[1].iov_base, 0, INT_SIZE);
	memset(msg->msg_iov[2].iov_base, 0, SEG_SIZE);
	memset(msg->msg_iov[3].iov_base, 0, INT_SIZE);

	memcpy(msg->msg_iov[0].iov_base, &type, 1);
	memcpy(msg->msg_iov[1].iov_base, numtobit(send_num), INT_SIZE);

	if(type == 'd' || type == 's'){
		memcpy(msg->msg_iov[2].iov_base, data, data_bytes);
		memcpy(msg->msg_iov[3].iov_base, numtobit(data_bytes), INT_SIZE);
	}else{
		strcpy(msg->msg_iov[2].iov_base, data);
	}

	return;
}

void free_msghdr(struct msghdr *msg){
	free(msg->msg_iov[0].iov_base);
	free(msg->msg_iov[1].iov_base);
	free(msg->msg_iov[2].iov_base);
	free(msg->msg_iov[3].iov_base);
	free(msg->msg_iov);
	free(msg);
	return;
}

char **readfile(char *filename, uint32_t *filesize, uint32_t *package_num, uint32_t **sendBytes){
	FILE *fp = fopen(filename, "rb");

	fseek(fp, 0, SEEK_END);
	*filesize = ftell(fp);
	rewind(fp);

	*package_num = (*filesize/SEG_SIZE) + 1;
	if(*filesize % SEG_SIZE != 0) (*package_num)++;

	*sendBytes = (uint32_t *)malloc(*package_num * sizeof(uint32_t));
	char **sendData = (char **)malloc(*package_num * sizeof(char *));

	sendData[0] = numtobit(*filesize);

	int i;
	for(i = 1; i < *package_num; i++){
		sendData[i] = (char *)malloc(SEG_SIZE * sizeof(char));
		memset(sendData[i], 0, SEG_SIZE);
		size_t n = fread(sendData[i], 1, SEG_SIZE, fp);
		(*sendBytes)[i] = n;
	}
	fclose(fp);
	return sendData;
}

void send_info(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen, char **sendData){
	struct msghdr *sendbuf, *recvbuf;
	sendbuf = init_msghdr(recv_addr, recvlen);
	set_msghdr(sendbuf, 's', 0, sendData[0], sizeof(sendData[0]));
	recvbuf = init_msghdr(recv_addr, recvlen);

	if(sendmsg(sockfd, sendbuf, 0) < 0){
		printf("send error\n");
		exit(1);
	}

	ualarm(SEND_TIMEO, 0);
	if(recvmsg(sockfd, recvbuf, 0) < 0){
		if(errno == EINTR){
			printf("ACK timeout\n");
			continue;
		}else{
			printf("recv error\n");
			exit(1);
		}
	}
	ualarm(0, 0);

	free_msghdr(sendbuf);
	free_msghdr(recvbuf);

	return;
}

void send_file(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen, char *filename){
	uint32_t filesize, package_num, *sendBytes, send_num = 1;
	struct msghdr *sendbuf, *recvbuf;
	char **sendData;

	sendData = readfile(filename, &filesize, &package_num, &sendBytes);

	send_info(sockfd, recv_addr, recvlen, sendData);
	sendbuf = init_msghdr(recv_addr, recvlen);
	recvbuf = init_msghdr(recv_addr, recvlen);

	while(send_num < package_num){
		set_msghdr(sendbuf, DATA, send_num, sendData[send_num], sendBytes[send_num]);

		ssize_t n = sendmsg(sockfd, sendbuf, 0);
		if(n < 0){
			printf("send error\n");
			exit(1);
		}

		ualarm(SEND_TIMEO, 0);
		n = recvmsg(sockfd, recvbuf, 0);
		if(n < 0){
			if(errno == EINTR){
				printf("ACK timeout\n");
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}
		ualarm(0, 0);

		uint32_t recv_num = bittonum((char *)recvbuf->msg_iov[1].iov_base);
		if(recv_num > send_num) send_num = recv_num;
	}

	set_msghdr(sendbuf, 'a', send_num, "close", sizeof("close"));
	if(sendmsg(sockfd, sendbuf, 0) < 0){
		printf("send error\n");
		exit(1);
	}

	free_msghdr(sendbuf);
	free_msghdr(recvbuf);
	for(send_num = 0; send_num < package_num; send_num++){
		free(sendData[send_num]);
	}
	free(sendData);
	free(sendBytes);

	return;
}

static void sig_alarm(int signo){
	return;
}

int main(int argc, char *argv[]){
	char *serverIP;
	int serverPORT;
	int sockfd;
	struct sockaddr_in server;
	struct addrinfo hints, *servinfo;

	memset(&hints, 0 sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_INET;
	if(getaddrinfo(argv[2], NULL, &hints, &servinfo) != 0){
		printf("getaddrinfo error\n");
		exit(1);
	}

	serverIP = argv[2];
	serverPORT = atoi(argv[3]);

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	inet_pton(AF_INET, serverIP, &server.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	signal(SIGALRM, sig_alarm);
	siginterrupt(SIGALRM, 1);

	send_file(sockfd, &server, servinfo->ai_addrlen, argv[1]);
	freeaddrinfo(servinfo);

	return 0;
}