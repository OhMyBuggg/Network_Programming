#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define BUFF_SIZE 1500

int datalen = 56;
int sockfd;
pid_t pid;
char send_buff[BUFF_SIZE];
int nsent = 0;
int verbose = 0;

struct proto {
	struct hostent *host;
	struct sockaddr *sasend;
	struct sockaddr *sarecv;
	socklen_t salen;
	int icmpproto;
} sock;

struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype);
char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

void tv_sub(struct timeval *out, struct timeval *in);

int proc_icmp(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv);
void send_icmp();

uint16_t in_cksum(uint16_t *addr, int len);

int main(int argc, char *argv[]) {
	char *host;
	struct addrinfo *addr_in;
	char *h;

	if(argc != 2) {
		printf("Usage: %s <>\n", argv[0]);
		exit(1);
	}

	host = argv[1];
	pid = getpid() & 0xFFFF;

	int sub_ip;
	for(sub_ip = 1; sub_ip <= 254; sub_ip++) {
		char ip[128];
		sprintf(ip, "%s.%d", host, sub_ip);

		addr_in = host_serv(ip, NULL, 0, 0);
		h = sock_ntop_host(addr_in->ai_addr, addr_in->ai_addrlen);

		//printf("PING %s (%s): %d data bytes\n",
		//	addr_in->ai_canonname ? addr_in->ai_canonname : h,
		//	h, datalen);

		sock.sasend = addr_in->ai_addr;
		sock.sarecv = calloc(1, addr_in->ai_addrlen);
		sock.salen = addr_in->ai_addrlen;
		sock.icmpproto = IPPROTO_ICMP;
		in_addr_t ip_addr = inet_addr(ip);
		sock.host = gethostbyaddr(&ip_addr, sizeof(ip_addr), AF_INET);

		char recv_buff[BUFF_SIZE];
		char control_buff[BUFF_SIZE];
		struct msghdr msg;
		
		struct iovec iov;
		ssize_t n;
		struct timeval tval, send_tval;

		sockfd = socket(sock.sasend->sa_family, SOCK_RAW, sock.icmpproto);
		int size = 60 * 1024;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
		struct timeval timeout_tv;
		timeout_tv.tv_sec = 3;
		timeout_tv.tv_usec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_tv, sizeof(timeout_tv));
		
		send_icmp(&send_tval);

		iov.iov_base = recv_buff;
		iov.iov_len = sizeof(recv_buff);
		msg.msg_name = sock.sarecv;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = control_buff;

		int result;
		struct timeval wait_tv;
		do {
			msg.msg_namelen = sock.salen;
			msg.msg_controllen = sizeof(control_buff);
			n = recvmsg(sockfd, &msg, 0);

			if(n < 0) {
				if(errno == EINTR) {
					continue;
				} else if(errno == EAGAIN || errno == EWOULDBLOCK) {
					result = 0;
					break;
				} else {
					puts("recvmsg error");
					exit(2);
				}
			}

			gettimeofday(&tval, NULL);
			result = proc_icmp(recv_buff, n, &msg, &tval);

			wait_tv = send_tval;
			tv_sub(&wait_tv, &tval);
		} while(!result && wait_tv.tv_sec < 3);

		if(!result) {
			printf("%s (%s) no response\n",
				ip, sock.host->h_name);
		}

		sleep(1);
	}
	return 0;
}

struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype) {
	int n;
	struct addrinfo hints, *res;
	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = family;
	hints.ai_socktype = socktype;

	if((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("host_serv error for %s, %s, %s\n",
			host == NULL ? "No host" : host,
			serv == NULL ? "No serv" : serv,
			gai_strerror(n));
		exit(1);
	}

	return res;
}

char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {
	static char str[128];
	if(sa->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *) sa;
		if(inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)))
			return str;
	}

	return NULL;
}

void tv_sub(struct timeval *out, struct timeval *in) {
	if((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

int proc_icmp(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv) {
	int hlen1, icmplen;
	double rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	char recv_ip[128];
	inet_ntop(AF_INET, &((struct sockaddr_in *) sock.sarecv)->sin_addr,
		recv_ip, sizeof(recv_ip));

	ip = (struct ip *) ptr;
	hlen1 = ip->ip_hl << 2;
	if(ip->ip_p != IPPROTO_ICMP) {
		puts("IPPROTO error");
		return;
	}

	icmp = (struct icmp *) (ptr + hlen1);
	if((icmplen = len - hlen1) < 8) {
		puts("hlen1 error");
		return;
	}

	if(icmp->icmp_type == ICMP_ECHOREPLY) {
		if(icmp->icmp_id != pid) {
			puts("pid error");
			return;
		}
		if(icmplen < 16) {
			puts("length error");
			return;
		}

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		//printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
		//	icmplen, recv_ip, icmp->icmp_seq, ip->ip_ttl, rtt);
		printf("%s (%s) RTT=%.3fms\n", recv_ip, sock.host->h_name, rtt);
		return 1;
	} else {
		return 0;
	}
}

void send_icmp() {
	int len;
	struct icmp *icmp;

	icmp = (struct icmp *) send_buff;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = nsent++;
	memset(icmp->icmp_data, 0xa5, datalen);
	gettimeofday((struct timeval *) icmp->icmp_data, NULL);

	len = 8 + datalen;
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

	sendto(sockfd, send_buff, len, 0, sock.sasend, sock.salen);
}

uint16_t in_cksum(uint16_t *addr, int len) {
	int nleft = len;
	uint32_t sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while(nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	if(nleft == 1) {
		*(u_char *) (&answer) = *(u_char *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}
