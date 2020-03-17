#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netdb.h>

#include <iostream>
#include <fstream>
#include <vector>

#define MAXLINE 300
#define MAXPKG 1024
#define SA	struct sockaddr
#define max(a, b) ({ __typeof__ (a) _a; __typeof__ (b) _b = (b); _a > _b ? _a : _b;})
#define min(a, b) ({ __typeof__ (a) _a; __typeof__ (b) _b = (b); _a < _b ? _a : _b;})

using namespace std;

char username[MAXLINE];
char filename[MAXLINE];
char input[MAXLINE];
int uppre=0, downpre=0;

class fileinfo{
	public:
	char filename[MAXLINE];
	ssize_t fsize;
	char *fcontent;
	int donesize;
	fileinfo(char *name, ssize_t size, char *content){
		bzero(filename, MAXLINE);
		memcpy(filename, name, MAXLINE);
		fsize = size;
		fcontent = (char*)malloc(size);
		bzero(fcontent, size);
		memcpy(fcontent, content, size);
		donesize = 0;
	}
	fileinfo(){
		fsize = 0;
		donesize = 0;
	}
};
vector<fileinfo > upfiles;
fileinfo downfile;

FILE* filefd = NULL;
FILE* downfd = NULL;

void err_sys(const char* x){	// Fatal error related to a system call
	perror(x);
	exit(1);
}

void err_quit(const char* x){	// Fatal error unrelated to a system call
	perror(x);
	exit(1);
}

/*****	Error Handling Wrappers	*****/
int Socket(int family, int type, int protocol){
	int sockfd;

	if((sockfd = socket(family, type, protocol)) < 0)
		err_sys("Socket Error");

	return sockfd;
}

int Connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen){
	int n;

	if((n = connect(sockfd, servaddr, addrlen)) < 0)
		err_sys("Connect Error");

	return n;
}

int Select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout){
	int fdcnt;

	if((fdcnt = select(maxfdp1, readset, writeset, exceptset, timeout)) < 0)
		err_sys("Select Error");

	return fdcnt;
}


void sendfile(int fd){
	int remain = upfiles[0].fsize - upfiles[0].donesize;
	ssize_t n = write(fd, upfiles[0].fcontent+upfiles[0].donesize, MAXLINE > remain ? remain : MAXLINE);
	upfiles[0].donesize += n;
	int prog = 20*upfiles[0].donesize / upfiles[0].fsize;
	if(prog != uppre){
		fflush(stdout);
		printf("Pid: %d Progress [", getpid());
		for(int i=0;i<20;i++)
			if(i < prog)
				cout<<"#";
			else
				cout<<" ";
		printf("]\r");
	}
	uppre = prog;
	
	if(upfiles[0].donesize >= (int)upfiles[0].fsize){
		cout<<endl;
		uppre = 0;
		free(upfiles[0].fcontent);
		cout<<"Pid: "<<getpid()<<" [Upload] "<<upfiles[0].filename<<" Finish!\n";
		upfiles.erase(upfiles.begin());
	}
}

void chat_cli(FILE *fp, int sockfd){
	int maxfdp1, stdineof, val;
	ssize_t n, nwritten;
	fd_set rset, wset;
	char pkg[MAXPKG];
	char compkg[MAXPKG];
	char *reads, *comr;
	int bufferspc = MAXPKG;
	//char to[MAXLINE], fr[MAXLINE];
	//char *toiptr, *tootpr, *friptr, *froptr;
	
	val = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, val|O_NONBLOCK);

	val = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDOUT_FILENO, F_SETFL, val|O_NONBLOCK);
    
    // val = fcntl(STDOUT_FILENO, F_GETFL, 0);
    // fcnctl(STDOUT_FILENO, F_SETTFL, val|O_NONBLOCK);

	while(write(sockfd, username, MAXLINE)<0){
		if(errno != EWOULDBLOCK){
			cout<<"error on sending username, errno: "<<errno<<endl;
			return;
		}
	} //send username to server

	//toiptr = tooptr = to;
	//friptr = froptr = fr;
	reads = &pkg[0];
	comr = &compkg[0];
	stdineof = 0;

	maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;
	for(;;){
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(sockfd, &rset);
		FD_SET(fileno(fp), &rset);
		if(upfiles.size()>0)
			FD_SET(sockfd, &wset);
		// if(stdineof==0 && toiptr == <&to[MAXLINE])
		// 	FD_SET(STDIN_FILENO, &rset);
		// if(friptr < &fr[MAXLINE])
		// 	FD_SET(sockfd, &rset);
		if(reads != pkg)
		 	FD_SET(sockfd, &rset);
		// if(froptr != friptr)
		// 	FD_SET(STDOUT_FILENO, &wset);

		Select(maxfdp1, &rset, &wset, NULL, NULL);

		if(FD_ISSET(sockfd, &wset)){
			sendfile(sockfd);
		}
		
		int n;
		
		bzero(input, MAXLINE);
		if(FD_ISSET(STDIN_FILENO, &rset)){	// input is readable
			if((n = read(STDIN_FILENO, input, MAXLINE)) < 0){
				if(errno != EWOULDBLOCK){
					cout<<"read from stdin error\n";
					return;
				}
			}
			else if(n==0){
				cout<<"stdin eof\n";
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
			}
			else{
				if(strncmp(input, "exit", 4)==0){
					close(sockfd);
					return;
				}
				else if(strncmp(input, "put ", 4)==0){
					//start a upload
					char filename[MAXLINE];
					sscanf(input, "put %s", &filename);
					filefd = fopen(filename, "rb");
					//FD_SET(sockfd, &wset);
					fseek(filefd, 0, SEEK_END);
					ssize_t filesize = ftell(filefd);
					fseek(filefd, 0, SEEK_SET);
					char *content;
					content = (char*)malloc(filesize);
					fread(content, 1, filesize, filefd);
					fileinfo newfile(filename, filesize, content);
					upfiles.push_back(newfile);
					char msg[MAXLINE];
					int size = (int)filesize;
					sprintf(msg, "put %s %d", filename, size);
					write(sockfd, msg, MAXLINE);
					cout<<"Pid: "<<getpid()<<" [Upload] "<<filename<<" Start!\n";
				}
				else if(strncmp(input, "sleep ", 6)==0){
					int sec;
					sscanf(input, "sleep %d", &sec);
					cout<<"Pid: "<<getpid()<<" The client starts to sleep.\n";
					for(int i=0;i<sec;i++){
						sleep(1);
						char out[20];
						bzero(out, 20);
						sprintf(out, "Pid: %d Sleep %d", getpid(), i+1);
						cout<<out<<endl;
					}
					cout<<"Pid: "<<getpid()<<" Client wakes up.\n";
				}
			}
		}
		
		
		if(FD_ISSET(sockfd, &rset)){	// socket is readable
			int readbyte;
			if(memcmp(compkg, pkg, MAXPKG)==0 || bufferspc <= 0){
				bzero(&pkg, MAXPKG);
				bzero(&compkg, MAXPKG);
				reads = pkg;
				comr = compkg;
				bufferspc = MAXPKG;
			}
			if(downfd == NULL)
				readbyte = MAXLINE;
			else{
				int remsz = downfile.fsize - downfile.donesize;
				if(remsz > MAXLINE)
					readbyte = MAXLINE;
				else
					readbyte = remsz;
			}
			if(readbyte > bufferspc) readbyte = bufferspc;
			n = read(sockfd, reads, readbyte);
			memcpy(comr, reads, n);
			comr += n;
			bufferspc -= n;
			if(n < 0){
				if(errno != EWOULDBLOCK){
					cout<<"read from socket error, errno: "<<errno<<endl;
					return;
				}
				else continue;
			}
			else if(n == 0){
				if(stdineof == 1)
					return;
				else
					return;
					//err_quit("Server Terminated prematurely");
			}
			else {
				// cout<<"read "<<n<<" bytes on socket\n";
				//friptr += n;
				if(strncmp(reads, "put ", 4)==0){
					
					//start a download
					char filename[MAXLINE];
                    bzero(filename, MAXLINE);
                    int size;
                    sscanf(reads, "put %s %d", &filename, &size);
                    string fname = filename;
                    memcpy(downfile.filename, filename, MAXLINE);
					downfile.donesize = 0;
					downfile.fsize = size; 
                    char path[MAXLINE];
                    bzero(path, MAXLINE);
                    sprintf(path, "./%s", filename);
                    downfd = fopen(path, "wb");
					cout<<"Pid: "<<getpid()<<" [Download] "<<filename<<" Start!\n";
				}
				else{
					write(fileno(downfd), reads, n);
					downfile.donesize += n;
					int prog = 20*downfile.donesize / downfile.fsize;
					if(prog != downpre){
						fflush(stdout);
						printf("Pid: %d Progress [", getpid());
						for(int i=0;i<20;i++)
							if(i < prog)
								cout<<"#";
							else
								cout<<" ";
						printf("]\r");
					}
					downpre = prog;
					
					if(downfile.donesize >= downfile.fsize){
						cout<<endl;
						fclose(downfd);
						downfd = NULL;
						cout<<"Pid: "<<getpid()<<" [Download] "<<downfile.filename<<" Finish!\n";
						downpre = 0;
					}
				}
				reads += n;
			}
		}
		
	}
}

int main(int argc, char **argv){
	char *servip;
	int servport;
	int sockfd;
	struct sockaddr_in servaddr;

	if(argc != 4){
		printf("Incorrect inputs. Usage: ./client <Server IP> <Port Number> <username>.\n");
		exit(1);
	}
	bzero(&username, MAXLINE);
	memcpy(username, argv[3], MAXLINE);
	cout<<"Welcome to the dropbox-like server: "<<username<<endl;

	servip = argv[1];
	servport = atoi(argv[2]);

//	printf("Server ip: %s\nServer port: %d\n", servip, servport);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	int socksendbufsize = 30000;
	//int sockrecvbufsize = 65536*2;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &socksendbufsize, sizeof(socksendbufsize));
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockrecvbufsize, sizeof(sockrecvbufsize));

	bzero(&servaddr, sizeof(servaddr));	// set bytes of servaddr to zero
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);
	inet_pton(AF_INET, servip, &servaddr.sin_addr);

	Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	chat_cli(stdin, sockfd);

	return 0;
}
