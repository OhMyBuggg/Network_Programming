#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

main(int argc, char *argv[]) {

           struct sockaddr_in ssaa, *sain;    /* input */
           struct sockaddr *sa;    /* input */
           char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	   sain = &ssaa;
	   sain->sin_len = sizeof(struct sockaddr_in);
	   sain->sin_family = AF_INET;
	   sain->sin_port = htons(80);
	   inet_aton("140.113.1.1", &sain->sin_addr); 
	   sa = (struct sockaddr *) sain;
           if (getnameinfo(sa, sa->sa_len, hbuf, sizeof(hbuf), sbuf,
               sizeof(sbuf), 0)) {
                   errx(1, "could not get numeric hostname");
                   /* NOTREACHED */
           }
           printf("host=%s, serv=%s\n", hbuf, sbuf);
}
