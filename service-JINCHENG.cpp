#include<sys/types.h>
#include<sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <errno.h>
using namespace std;
void fun(int pid)
{
    int p = waitpid(0,0,WNOHANG);
    std::cout<<p <<"end"<<endl;
}


int main()
{
    sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_port = htons(9090);
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    extern int errno;
    int fd = socket(AF_INET,SOCK_STREAM,0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    if (fd>0)
    {
        bind(fd, (struct sockaddr*)&add, sizeof (add));
        listen(fd,60);
        sockaddr_in in;

        socklen_t len = sizeof in;
        int clientid;
        int childid;
        while (1)
        {

            clientid = accept(fd,(struct sockaddr*)&in, &len);
            if (clientid==-1 && errno == EINTR)
                continue;


            childid = fork();
            if (childid>0)
            {//父进程
                close(clientid);
                struct sigaction sig;
                sigemptyset(&sig.sa_mask);
                sig.sa_flags = 0;
                sig.sa_handler = fun;
                sigaction(SIGCHLD,&sig,0);

            }
            else if(!childid)
            {//子进程
                close(fd);
                while (1)
                {
                    char buf[1024];
                    int temp = read(clientid,buf,1024);
                    if (temp>0)
                    {
                        std::cout<<getpid()<<" get data :";
                        std::cout.write(buf,temp)<<endl;
                    } else {
                        close(clientid);
                        exit(0) ;
                    }
                }
            }
            else
            {
                cout<<"fock error"<<endl;
            }
        }
    }
}
