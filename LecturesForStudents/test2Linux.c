#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {

           struct sockaddr_in ssaa, *sain;    /* input */
           struct sockaddr *sa;    /* input */
           char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	   sain = &ssaa;
	   sain->sin_family = AF_INET;
	   sain->sin_port = htons(80);
	   inet_aton("140.113.1.1", &sain->sin_addr); 
	   sa = (struct sockaddr *) sain;
           if (getnameinfo(sa, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf,
               sizeof(sbuf), 0)) {
                   printf("could not get hostname\n");
           }
           printf("host=%s, serv=%s\n", hbuf, sbuf);
}
