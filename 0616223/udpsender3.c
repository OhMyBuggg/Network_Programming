#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>

#define SEG_SIZE 1000
#define INT_SIZE 4
#define SEND_TIMEO 250000

char *numtobit(uint32_t num){
	char *bit = (char *)malloc(INT_SIZE * sizeof(char));
	memset(bit, 0, INT_SIZE);

	int i, j, offset = 31;
	for(i = 0; i < 4; i++){
		uint32_t c_offset = 7;
		for(j = 0; j < 8; j++){
			uint32_t temp = (num & (1 << offset)) >> offset;
			bit[i] |= temp << c_offset;
			offset--;
			c_offset--;
		}
	}

	return bit;
}

uint32_t bittonum(char *bit){
	uint32_t num = 0;

	int i, j, offset = 31;
	for(i = 0; i < 4; i++){
		uint32_t c_offset = 7;
		for(j = 0; j < 8; j++){
			uint32_t temp = (bit[i] & (1 << c_offset)) >> c_offset;
			num |= temp << offset;
			offset--;
			c_offset--;
		}
	}

	return num;
}

struct msghdr* init_msghdr(struct sockaddr_in *sock_addr, socklen_t addrlen){
	struct msghdr *msg = (struct msghdr *)malloc(sizeof(struct msghdr));
	memset(msg, 0, sizeof(struct msghdr));

	struct iovec *iov = (struct iovec *)malloc(3 * sizeof(struct iovec));

	char *str_sn = (char *)malloc(INT_SIZE * sizeof(char));
	char *data = (char *)malloc(SEG_SIZE * sizeof(char));
	char *data_bytes = (char *)malloc(INT_SIZE * sizeof(char));
	memset(str_sn, 0, INT_SIZE);
	memset(data, 0, SEG_SIZE);
	memset(data_bytes, 0, INT_SIZE);

	iov[0].iov_base = str_sn;
	iov[0].iov_len = INT_SIZE;
	iov[1].iov_base = data;
	iov[1].iov_len = SEG_SIZE;
	iov[2].iov_base = data_bytes;
	iov[2].iov_len = INT_SIZE;

	msg->msg_iov = iov;
	msg->msg_iovlen = 3;
	msg->msg_name = sock_addr;
	msg->msg_namelen = addrlen;

	return msg;
}

void set_msghdr(struct msghdr *msg, uint32_t send_num,char *data, uint32_t data_bytes){
	memset(msg->msg_iov[0].iov_base, 0, INT_SIZE);
	memset(msg->msg_iov[1].iov_base, 0, SEG_SIZE);
	memset(msg->msg_iov[2].iov_base, 0, INT_SIZE);

	memcpy(msg->msg_iov[0].iov_base, numtobit(send_num), INT_SIZE);
	memcpy(msg->msg_iov[1].iov_base, data, data_bytes);
	memcpy(msg->msg_iov[2].iov_base, numtobit(data_bytes), INT_SIZE);

	return;
}

void free_msghdr(struct msghdr *msg){
	free(msg->msg_iov[0].iov_base);
	free(msg->msg_iov[1].iov_base);
	free(msg->msg_iov[2].iov_base);
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
	set_msghdr(sendbuf, 0, sendData[0], sizeof(sendData[0]));
	recvbuf = init_msghdr(recv_addr, recvlen);

	uint32_t send_num = 0;
	while(send_num < 1){
		if(sendmsg(sockfd, sendbuf, 0) < 0){
			printf("send error\n");
			exit(1);
		}
		if(recvmsg(sockfd, recvbuf, 0) < 0){
			if(errno == EWOULDBLOCK){
				printf("ACK timeout1\n");
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}

		uint32_t recv_num = bittonum(recvbuf->msg_iov[0].iov_base);
		if(recv_num > send_num) send_num++;
	}

	free_msghdr(sendbuf);
	free_msghdr(recvbuf);

	return;
}

void send_file(int sockfd, struct sockaddr_in *recv_addr, socklen_t recvlen, char *filename){
	uint32_t filesize, package_num, *sendBytes, send_num = 1;
	struct msghdr *sendbuf, *recvbuf;
	char **sendData;

	sendData = readfile(filename, &filesize, &package_num, &sendBytes);
	printf("***************read file done***************\n");

	send_info(sockfd, recv_addr, recvlen, sendData);
	printf("***************send info done***************\n");
	sendbuf = init_msghdr(recv_addr, recvlen);
	recvbuf = init_msghdr(recv_addr, recvlen);

	while(send_num < package_num){
		set_msghdr(sendbuf, send_num, sendData[send_num], sendBytes[send_num]);
		printf("send %u package\n", send_num);
		ssize_t n = sendmsg(sockfd, sendbuf, 0);
		if(n < 0){
			printf("send error\n");
			exit(1);
		}

		n = recvmsg(sockfd, recvbuf, 0);
		if(n < 0){
			if(errno == EWOULDBLOCK){
				printf("loss %d ACK\n", send_num);
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}

		uint32_t recv_num = bittonum((char *)recvbuf->msg_iov[0].iov_base);
		printf("recv %u ack\n", recv_num);
		if(recv_num > send_num) send_num = recv_num;
	}
	printf("***************send file done***************\n");

	set_msghdr(sendbuf, send_num, "c", sizeof("c"));
	for(int j = 0; j < 5; j++){
		if(sendmsg(sockfd, sendbuf, 0) < 0){
			printf("send error\n");
			exit(1);
		}
	}
	printf("***************send ack done***************\n");
	printf("closing...\n");

	free_msghdr(sendbuf);
	free_msghdr(recvbuf);
	for(send_num = 0; send_num < package_num; send_num++){
		free(sendData[send_num]);
	}
	free(sendData);
	free(sendBytes);

	return;
}

int main(int argc, char *argv[]){

	/*uint32_t a = 2;
	uint32_t temp = bittonum(numtobit(a));
	printf("num is %u\n", temp);
	return 0;*/

	char *serverIP;
	int serverPORT;
	int sockfd;
	struct sockaddr_in server;
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
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
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = SEND_TIMEO;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	send_file(sockfd, &server, servinfo->ai_addrlen, argv[1]);
	freeaddrinfo(servinfo);

	return 0;
}