#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define QSIZE 8
#define MAXDG 4096

int sockfd, iget, iput, nqueue;
socklen_t clilen;
static long cntread[QSIZE+1];

struct DG {
	void *dg_data;
	size_t dg_len;
	struct sockaddr *dg_sa;
	socklen_t dg_salen;
}dg[QSIZE];

static void sig_io(int signo) {
	ssize_t len;
	int nread;
	struct DG *ptr;

	for(nread = 0;;) {
		if(nqueue >= QSIZE)
			printf("receive overflow\n");

		ptr = &dg[iput];
		ptr->dg_salen = clilen;
		len = recvfrom(sockfd, ptr->dg_data, MAXDG, 0, ptr->dg_sa, &ptr->dg_salen);
		if(len < 0) {
			if(errno = EWOULDBLOCK)break;
			else {
				printf("recv error\n");
				exit(1);
			}
		}
		printf("recv msg: %s\n", ptr->dg_data);
		ptr->dg_len = len;
		nread++;
		nqueue++;
		if(++iput >= QSIZE)
			iput = 0;
	}
	cntread[nread]++;
}

void echo_server(int sockfd_arg, struct sockaddr *pcliaddr, socklen_t clilen_arg) {
	int i;
	const int on = 1;
	sigset_t zeromask, newmask, oldmask;

	sockfd = sockfd_arg;
	clilen = clilen_arg;

	for(i = 0; i < QSIZE; i++){
		dg[i].dg_data = (void *)malloc(MAXDG);
		dg[i].dg_sa = (struct sockaddr *)malloc(clilen);
		dg[i].dg_salen = clilen;
	}

	iget = iput = nqueue = 0;
	signal(SIGIO, sig_io);
	fcntl(sockfd, F_SETOWN, getpid());
	ioctl(sockfd, FIOASYNC, &on);
	ioctl(sockfd, FIONBIO, &on);

	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigemptyset(&oldmask);
	sigaddset(&newmask, SIGIO);

	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	for(;;) {
		while(nqueue == 0)
			sigsuspend(&zeromask);

		sigprocmask(SIG_SETMASK, &oldmask, NULL);

		sendto(sockfd, dg[iget].dg_data, dg[iget].dg_len, 0, dg[iget].dg_sa, dg[iget].dg_salen);
		if(++iget >= QSIZE)
			iget = 0;

		sigprocmask(SIG_BLOCK, &newmask, &oldmask);
		nqueue--;
	}
}

int main(int argc, char *argv[]) {
	int serverPORT;
	struct sockaddr_in server, sender;

	serverPORT = atoi(argv[1]);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("bind failed\n");
		exit(1);
	}

	echo_server(sockfd, (struct sockaddr *)&server, sizeof(server));

	return 0;
}