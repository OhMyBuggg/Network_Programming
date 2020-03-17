#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {

	while(--argc > 0) {
		int i = argc;
		if(isdigit(argv[i][0])) {
			struct sockaddr_in ssaa, *sain;
			struct sockaddr *sa;
			char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];

			sain = &ssaa;
			sain->sin_family = AF_INET;
			sain->sin_port = htons(80);
			inet_aton(argv[i], &sain->sin_addr);
			sa = (struct sockaddr *)sain;
			if(getnameinfo(sa, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), 0)) {
				printf("could not get hostname\n");
			} else {
				printf("hostname: %s, serv: %s\n", hbuf, sbuf);
			}
		} else {
			struct addrinfo hints, *servinfo, *p;
			int rv;

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			if((rv = getaddrinfo(argv[i], "http", &hints, &servinfo)) != 0){
				printf("error no is: %d\n\n", rv);
				continue;
			}

			for(p = servinfo; p != NULL; p = p->ai_next) {
				char IP[INET_ADDRSTRLEN];
				strcpy(IP, inet_ntoa(((struct sockaddr_in *)p->ai_addr)->sin_addr));
				printf("IP address is %s\n", IP);
			}
		}
		printf("\n");
	}
	return 0;
}