#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define SEG_SIZE 1000
#define INT_SIZE 4
#define RECV_TIMEO 150000

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

void set_msghdr(struct msghdr *msg, uint32_t send_num, char *data, uint32_t data_bytes){
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


void writefile(const char *filename, char **buffer, int *buflen, int package_num){
	FILE *fp = fopen(filename, "wb");

	int i;
	for(i = 1; i < package_num; i++){
		size_t n = fwrite(buffer[i], 1, buflen[i], fp);
		if(n < 0){
			printf("fwrite error\n");
			exit(1);
		}
	}
	fclose(fp);
	return;
}

void recv_info(int sockfd, struct sockaddr_in *send_addr, uint32_t *filesize, uint32_t *package_num){
	struct msghdr *sendbuf, *recvbuf;
	sendbuf = init_msghdr(send_addr, sizeof(struct sockaddr_in));
	recvbuf = init_msghdr(send_addr, sizeof(struct sockaddr_in));
	ssize_t n;
	uint32_t recv_num = 0;

	while(recv_num < 1){
		ualarm(RECV_TIMEO, 0);
		if((n = recvmsg(sockfd, recvbuf, 0)) < 0){
			if(errno == EINTR){
				if(recv_num)printf("recv timeout1\n");
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}
		ualarm(0, 0);

		*filesize = bittonum((char *)recvbuf->msg_iov[1].iov_base);
		*package_num = (*filesize / SEG_SIZE) + 1;
		if(*filesize % SEG_SIZE != 0)(*package_num)++;
		recv_num++;
	}

	set_msghdr(sendbuf, recv_num, "a", sizeof("a"));
	n = sendmsg(sockfd, sendbuf, 0);
	if(n < 0){
		printf("send error\n");
		exit(1);
	}
}

void recv_file(int sockfd, struct sockaddr_in *send_addr, char *filename){
	uint32_t filesize, package_num, recv_num = 1;
	int i;
	struct msghdr *sendbuf, *recvbuf;
	printf("***************wait for sending***************\n");

	recv_info(sockfd, send_addr, &filesize, &package_num);
	printf("***************recv info done***************\n");

	char **fileBuffer = (char **)malloc(package_num * sizeof(char *));
	uint32_t *fileBytes = (uint32_t *)malloc(package_num * sizeof(uint32_t));
	uint32_t *recv_table = (uint32_t *)malloc(package_num * sizeof(uint32_t));

	for(i = 0; i < package_num; i++){
		recv_table[i] = 0;
		fileBuffer[i] = (char *)malloc(SEG_SIZE * sizeof(char));
		memset(fileBuffer[i], 0, SEG_SIZE);
	}

	sendbuf = init_msghdr(send_addr, sizeof(struct sockaddr_in));
	recvbuf = init_msghdr(send_addr, sizeof(struct sockaddr_in));

	while(recv_num < package_num){
		ualarm(RECV_TIMEO, 0);
		ssize_t n = recvmsg(sockfd, recvbuf, 0);
		if(n < 0){
			if(errno == EINTR){
				printf("loss %d package\n", recv_num);
				set_msghdr(sendbuf, recv_num, "a", sizeof("a"));
				n = sendmsg(sockfd, sendbuf, 0);
				if(n < 0){
					printf("send error\n");
					exit(1);
				}
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}
		ualarm(0, 0);

		uint32_t send_num = bittonum((char *)recvbuf->msg_iov[0].iov_base);
		printf("recv %u package\n", send_num);
		if(recv_table[send_num] == 0){
			recv_table[send_num] = 1;
			fileBytes[send_num] = bittonum((char *)recvbuf->msg_iov[2].iov_base);
			memcpy(fileBuffer[send_num], (char *)recvbuf->msg_iov[1].iov_base, fileBytes[send_num]);
		}
		while(recv_table[recv_num] == 1)recv_num++;

		set_msghdr(sendbuf, recv_num, "a", sizeof("a"));
		n = sendmsg(sockfd, sendbuf, 0);
		printf("send %u ack\n", recv_num);
		if(n < 0){
			printf("send error\n");
			exit(1);
		}
	}
	printf("***************recv file done***************\n");

	for(i = 0; i < 20; i++){
		ualarm(RECV_TIMEO, 0);
		if(recvmsg(sockfd, recvbuf, 0) < 0){
			if(errno == EINTR){
				printf("recv timeout3\n");
				continue;
			}else{
				printf("recv error\n");
				exit(1);
			}
		}
		ualarm(0, 0);

		if(strcmp((char *)recvbuf->msg_iov[1].iov_base, "c") == 0){
			printf("***************recv close done***************\n");
			break;
		}
	}

	writefile(filename, fileBuffer, fileBytes, package_num);
	printf("***************write file done***************\n");
	printf("closing...\n");
	free_msghdr(sendbuf);
	free_msghdr(recvbuf);
	free(recv_table);
	free(fileBytes);
	for(i = 0; i < package_num; i++){
		free(fileBuffer[i]);
	}
	free(fileBuffer);

	return;
}

static void sig_alarm(int signo){
	return;
}

int main(int argc, char *argv[]){
	signal(SIGALRM, sig_alarm);
	siginterrupt(SIGALRM, 1);

	int sockfd;
	int serverPORT;
	struct sockaddr_in server, sender;

	serverPORT = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
		printf("bind failed\n");
		exit(1);
	}

	recv_file(sockfd, &sender, argv[1]);

	return 0;
}