#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

main(int argc, char *argv[]) {

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("www.apple.com", "http", &hints, &servinfo)) != 0) {
  		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
	   printf("IP address is %s\n", inet_ntoa(((struct sockaddr_in *) p->ai_addr)->sin_addr)); 

	}
}
