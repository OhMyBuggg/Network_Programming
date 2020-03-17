#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

static int sockfd;
static void *read_socket(void *arg);

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s <SERVER IP> <SERVER PORT>\n", argv[0]);
		exit(1);
	}

	struct hostent *host = gethostbyname(argv[1]);

	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(addr));
	memcpy(&addr.sin_addr, host->h_addr, host->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));

	pthread_t t;
	pthread_create(&t, NULL, read_socket, NULL);

	/* Main thread: Read from stdin and send to socket. */
	int num;
	while(scanf("%d", &num) != EOF) {
		char buffer[1000];
		sprintf(buffer, "%d", num);
		printf("send %s\n", buffer);
		write(sockfd, buffer, 1000);

		int rand_sec = rand() % 4 + 1;
		printf("wait %ds\n", rand_sec);
		sleep(rand_sec);
	}

	close(sockfd);
}

void *read_socket(void *arg) {
	/* Work thread: Read from socket and print to screen. */
	while(1) {
		char buffer[1000];
		int n = read(sockfd, buffer, 1000);
		
		if(n == 0) {
			puts("Server has terminated the connection.");
			exit(0);
		}
		printf("recv %s\n", buffer);
	}
}
