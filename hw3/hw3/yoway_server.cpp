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
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define MAXLINE 300
#define MAXPKG 1024
#define BL 10

using namespace std;

bool check(char);
void clear(char* an);
char ans[MAXLINE];
//int user_count = 0;
// int money[BL];
// int com_count[BL];
// char history[BL][MAXPKG][MAXLINE];
int maxdbi=0;
int db_count = 0;
int client_db[BL];
FILE* client_fd[BL];
FILE* client_outfd[BL];
int client_datalist[BL];
class user{
    public:
    string username;
    vector<string > files;
    vector<int > fsize;
    vector<int > remain;
    user(string newuser=""){
        username = newuser;
        // bzero(files, MAXLINE*MAXLINE);
        // bzero(fsize, MAXLINE);
    }
};
vector<user > users;
class client{
    public:
    string tmpname;
    string username;
    vector<string > files;
    vector<int > fsize;
    int remain;
    vector<int > lacks;
    char* buffer;
    bool download;
    client(string newuser=""){
        username = newuser;
        download = 0;
        remain = 0;
        // bzero(files, MAXLINE*MAXLINE);
        // bzero(fsize, MAXLINE);
    }
};
vector<client > clients(BL);

//char* bank(int fd, int id, char *line);
void checkcomplete(int maxi){
    ////cout<<"checkcomplete called with maxi "<<maxi<<endl;
    for(int i=0;i<=maxi;i++){
        int usrid = -1;
        for(int j=0;j<users.size();j++){
            if(clients[i].username==users[j].username){
                ////cout<<"find client "<<i<<"'s name\n";
                usrid = j;
                break;
            }
        }
        if(usrid < 0){
            ////cout<<"error: client "<<i<<" has no corresponding user data\n";
            return;
        }
        // if(clients[i].files.size()==users[usrid].files.size())
        //     continue; //correct file number(too lazy to check if data wrong)
        // //cout<<"client "<<i<<" lack some files\n";
        for(int j=0;j<users[usrid].files.size();j++){
            int k;
            for(k=0;k<clients[i].files.size();k++){
                if(clients[i].files[k] == users[usrid].files[j]){
                    ////cout<<"client "<<i<<" has "<<clients[i].files[k]<<endl;
                    break;
                }
            }
            if(k == clients[i].files.size()){
                //cout<<"currently maxi: "<<maxi<<endl;
                
                bool hasstored = 0;
                for(int x=0;x<clients[i].lacks.size();x++){
                    if(clients[i].lacks[x]==j){
                        //cout<<"client "<<i<<" doesn't have "<<users[usrid].files[j]<<" but has stored before\n";
                        hasstored = 1;
                        break;
                    }
                }
                if(hasstored)continue;
                //cout<<"client "<<i<<" doesn't have "<<users[usrid].files[j]<<endl;
                //cout<<"index "<<j<<" stored\n";
                clients[i].lacks.push_back(j);
            }
        }
        ////cout<<"client "<<i<<" check done\n";
    }
}

int main(int argc, char** argv){
    if(argc!=2)
        exit(-1);
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char buff[MAXPKG];
    char combuff[MAXPKG];
    char *bufptr, *comptr;
    bufptr = buff;
    comptr = combuff;
    bzero(&buff, MAXPKG);
    bzero(&combuff, MAXPKG);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
        printf("bind error\n");
        exit(-1);
    }

    listen(listenfd, BL);

    int flag = fcntl(listenfd, F_GETFL, 0);
    fcntl(listenfd, F_SETFL, flag|O_NONBLOCK);
    
    flag = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDOUT_FILENO, F_SETFL, flag|O_NONBLOCK);
    
    // flag = fcntl(STDOUT_FILENO, F_GETFL, 0);
    // fcntl(STDOUT_FILENO, F_SETFL, flag|O_NONBLOCK);

    int maxfd, maxi = -1;
    fd_set rset, raset, wset, waset;
    int client[BL];
    maxfd = listenfd;
    for(int i=0;i<BL;i++){
        client[i]=-1;
        client_db[i]=-1;
        client_datalist[i]=-1;
    }
    FD_ZERO(&raset);
    FD_ZERO(&rset);
    FD_ZERO(&waset);
    FD_ZERO(&wset);
    FD_SET(listenfd, &raset);
    FD_SET(listenfd, &waset);
    int nready;
    // bzero(buff, sizeof(buff));
    // bzero(history, sizeof(history));
    // memset(money, 0, sizeof(money));
    // memset(com_count, 0, sizeof(com_count));
    
    for(;;){
        rset = raset;
        wset = waset;
        nready = select(maxfd+1, &rset, &wset, NULL, NULL);
        ////cout<<"hohohoho\n";
        if(FD_ISSET(listenfd, &rset)){
            //cout<<"listen socket\n";
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

            char uname[MAXPKG];
            bzero(uname, MAXPKG);
            if(read(connfd, uname, MAXPKG)<0){
                
                    //cout<<"read name error, errno: "<<errno<<endl;
            }

            int flag = fcntl(connfd, F_GETFL, 0);
            fcntl(connfd, F_SETFL, flag|O_NONBLOCK);

	        int socksendbufsize = 30000;
	        //int sockrecvbufsize = 65536*2;
	        setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, &socksendbufsize, sizeof(socksendbufsize));
	        //setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, &sockrecvbufsize, sizeof(sockrecvbufsize));

            string name = uname;

            int i;
            for(i=0;i<BL;i++){
                if(client[i]<0){
                    client[i] = connfd;
                    break;
                }
            }
            if(i==BL){
                printf("too many clients\n");
                exit(-1);
            }
            if(connfd > maxfd)
                maxfd = connfd;
            
            int istore = i;
            for(i=0;i<users.size();i++){
                if(users[i].username == name){
                    client_db[istore] = i;
                    break;
                }
            }
            if(i==users.size()){
                ////cout<<"new user\n";
                user newuser(name);
                users.push_back(newuser);
                char newdir[MAXLINE];
                bzero(newdir, MAXLINE);
                sprintf(newdir, "%s", uname);
                
                mkdir(newdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                client_db[istore] = i;
            }
            clients[istore].username = name;
            
            FD_SET(connfd, &raset);
            FD_SET(connfd, &waset);
            if(istore > maxi)
                maxi = istore;
            
            // com_count[i]=0;
            // money[i]=0;
            // bzero(&history[i], sizeof(history[i]));
            //cout<<"new client added\n";
            if(--nready<=0)
                continue;
        }
        ////cout<<"start check\n";
        checkcomplete(maxi);
        ////cout<<"check done\n";
 
        int sockfd;
        for(int i=0;i<=maxi;i++){
            if((sockfd = client[i]) < 0)
                continue;
            if(FD_ISSET(sockfd, &rset)){
                int n;
                if(memcmp(combuff, buff, MAXPKG)==0){
                    bzero(&combuff, MAXPKG);
                    bzero(&buff, MAXPKG);
                    bufptr = buff;
                    comptr = combuff;
                }
                int plc = client_datalist[i];
                int readbyte = client_fd[i] == NULL ? MAXLINE : 
                (users[client_db[i]].remain[plc] < MAXLINE ? users[client_db[i]].remain[plc] : MAXLINE);
                ////cout<<"max read byte this ite: "<<readbyte<<endl;
                if ((n = read(sockfd, bufptr, readbyte))>0){
                    //cout<<"socket input "<<n<<" bytes\n";
                    memcpy(comptr, bufptr, n);
                    comptr += n;
                    if(strncmp(bufptr, "put ", 4)==0){
                        //cout<<"start a upload\n";
                        char filename[MAXLINE];
                        bzero(filename, MAXLINE);
                        int size;
                        sscanf(bufptr, "put %s %d", &filename, &size);
                        string fname = filename;
                        clients[i].tmpname = fname;
                        
                        clients[i].fsize.push_back(size);
                        int userid = client_db[i];
                        
                        users[userid].fsize.push_back(size);
                        users[userid].remain.push_back(size);
                        client_datalist[i] = users[userid].fsize.size()-1; 
                        // this client is currently dealing with data at this place for this user
                        client_fd[i] = NULL;
                        char path[MAXLINE];
                        bzero(path, MAXLINE);
                        sprintf(path, "./%s/%s", users[userid].username.c_str(), filename);
                        client_fd[i] = fopen(path, "wb");
                    }
                    else{
                            //cout<<"read "<<n<<" bytes of data\n";
                            write(fileno(client_fd[i]), bufptr, n);
                            int rm = users[client_db[i]].remain[client_datalist[i]] -= n;
                            if(rm <= 0){
                                int userid = client_db[i];
                                string fname = clients[i].tmpname;
                                users[userid].files.push_back(fname);
                                clients[i].files.push_back(fname);
                                client_datalist[i] = -1;
                                //cout<<"read eof\n";
                                fclose(client_fd[i]);
                                client_fd[i] = NULL;
                                clients[i].tmpname = "";
                                //cout<<"download "<<fname<<" done\n";
                            }
                    }
                }
                else{
                    if(errno = EWOULDBLOCK)
                        continue;
                    else{
                        //cout<<"error on socket read, errno: "<<errno<<endl;
                    }
                    close(sockfd);
                    FD_CLR(sockfd, &raset);
                    FD_CLR(sockfd, &waset);
                    client[i]=-1;
                    client_db[i]=-1;
                    client_datalist[i]=-1;
                }
                bufptr += n;
            }
            if(FD_ISSET(sockfd, &wset)){
                if(clients[i].lacks.size()==0)
                    continue;
                if(clients[i].download==0){
                    //cout<<"client "<<i<<" has "<<clients[i].lacks.size()<<" data not yet download\n";
                    //start a client download
                    int fileid = clients[i].lacks[0];
                    int usrid = client_db[i];
                    string filename = users[usrid].files[fileid];
                    int filesize = users[usrid].fsize[fileid];
                    clients[i].files.push_back(filename);
                    clients[i].fsize.push_back(filesize);
                    clients[i].remain = filesize;
					char msg[MAXLINE];
					sprintf(msg, "put %s %d", filename.c_str(), filesize);
                    write(sockfd, msg, MAXLINE);
                    //cout<<"start download: "<<msg<<endl;
                    char path[MAXLINE];
                    bzero(path, MAXLINE);
                    sprintf(path, "./%s/%s", clients[i].username.c_str(), filename.c_str());
                    ////cout<<"path: "<<path;
					client_outfd[i] = fopen(path, "rb");
					clients[i].buffer = (char*)malloc(filesize);
                    
					fread(clients[i].buffer, 1, filesize, client_outfd[i]);
                    
                    clients[i].download = 1;
                }
                else{
                    int id = clients[i].fsize.size()-1;
                    int remain = clients[i].remain;
                    int done = clients[i].fsize[id] - remain;
                    int n = write(sockfd, clients[i].buffer + done, remain > MAXLINE ? MAXLINE : remain);
                    clients[i].remain -= n;
                    if(clients[i].remain <= 0){
                        free(clients[i].buffer);
                        fclose(client_outfd[i]);
                        client_outfd[i] = NULL;
                        clients[i].lacks.erase(clients[i].lacks.begin());
                        clients[i].download = 0;
                    }
                }
                
            }
            if(--nready<=0)
                    break;
        }
    }
}
