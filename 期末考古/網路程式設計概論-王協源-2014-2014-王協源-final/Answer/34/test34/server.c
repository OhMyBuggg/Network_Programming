#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../writeline_r.h"

void *worker(void *);

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Usage: %s <SERVER PORT>\n", argv[0]);
		exit(1);
	}

	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		puts("Cannot bind!");
		exit(2);
	}
	listen(sockfd, 1024);

	while(1) {
		int *clientfd = malloc(sizeof(int));
		*clientfd = accept(sockfd, NULL, NULL);

		pthread_t t;
		pthread_create(&t, NULL, worker, clientfd);
		pthread_detach(t);
	}
}

void *worker(void *arg) {
	int *clientfd = (int *) arg;

	/* Set timeout */
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(*clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	int sum = 0;
	enum State {
		NO_RECEIVE,
		RECEIVE_ONE
	} state = NO_RECEIVE;
	while(1) {
		struct sockaddr recvaddr;
		socklen_t recvlen = sizeof(recvaddr);
		char buffer[1000];

		int n = read(*clientfd, buffer, 1000);
		if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			/* Timed out */
			state = NO_RECEIVE;
		} else if(n > 0) {
			int num;
			sscanf(buffer, "%d\n", &num);

			if(state == RECEIVE_ONE) {
				sum += num;
				state = NO_RECEIVE;

				sprintf(buffer, "%d\n", sum);
				int i;
				for(i=0; buffer[i] != '\0'; i++) {
					writeline_r(*clientfd, buffer[i]);
				}
			} else {
				sum = num;
				state = RECEIVE_ONE;
			}
		} else if(n == 0) {
			break;
		}
	}

	close(*clientfd);
}
