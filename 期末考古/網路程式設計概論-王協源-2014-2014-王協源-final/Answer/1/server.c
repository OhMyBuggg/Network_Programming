#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>

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

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		puts("Cannot bind!");
		exit(2);
	}
	listen(sockfd, 1024);

	/* Set timeout */
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	int sum = 0;
	enum State {
		NO_RECEIVE,
		RECEIVE_ONE
	} state = NO_RECEIVE;
	while(1) {
		struct sockaddr recvaddr;
		socklen_t recvlen = sizeof(recvaddr);
		char buffer[1000];

		int n = recvfrom(sockfd, buffer, 1000, 0, &recvaddr, &recvlen);
		if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			/* Timed out */
			state = NO_RECEIVE;
		} else if(n > 0) {
			int num;
			sscanf(buffer, "%d", &num);

			if(state == RECEIVE_ONE) {
				sum += num;
				state = NO_RECEIVE;

				sprintf(buffer, "%d", sum);
				sendto(sockfd, buffer, 1000, 0, &recvaddr, recvlen);
			} else {
				sum = num;
				state = RECEIVE_ONE;
			}
		}
	}
}
