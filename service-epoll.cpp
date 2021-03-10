//
// Created by feng on 2021/3/5.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <errno.h>
#include <sys/epoll.h>

using namespace std;
/*
void fun(int pid)
{
    int p = waitpid(0,0,WNOHANG);
    std::cout<<p <<"end"<<endl;
}
*/

#define PORT 9090
#define NUMS 60
int sentdata(int src_id, int dest_id, char *data)
{
    // 查找id是否在线，
    // 如果没在线就缓存到redis
    // 如果在线就直接发送大id对应的fd上。
    // 如果发送失败，就继续缓存在redis上
}
void dealData(char *data, int length)
{
    //判断类型，和用户方向。
    std::cout<<"read finish, now deal"<<std::endl;
    char buf[8];
    memcpy(buf,data,8);
    data+=8;
    int src_id = stoi(buf);
    memcpy(buf,data,8);
    data+=8;
    int dest_id = stoi(buf);

    std::cout.write(data,length)<<std::flush;
}

void setSockNonBlock(int sock) {
    int flags;
    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }
}
enum TYPE
{
    TEXT =1,
    PICTURE,
    SOUND,
    VIDEO,
    OTHER
};
#include <math.h>
int fun(int n)
{
    if (n==1)
    {
        return 1;
    }

    return n/int(pow(n,0.5))+fun(n%int(pow(n,0.5)));
}


// TODO 多线程或者多进程，主线accept，副线read

void init(int id, int fd)
{
    // 每当fd连接时，判断其对应的id是否在redis中有缓存，如果有缓存，就先处理缓存的数据
}
int main()
{

    char* buf ;
    char index[3];
    sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_port = htons(PORT);
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    extern int errno;
    int service_fd = socket(AF_INET,SOCK_STREAM,0);
    struct epoll_event ev, events[NUMS];


    int epoll_fd = epoll_create(1);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(service_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    setSockNonBlock(service_fd);

    ev.data.fd = service_fd;
    ev.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, service_fd, &ev);


    while (service_fd>0)
    {
        bind(service_fd, (struct sockaddr*)&add, sizeof (add));
        listen(service_fd,NUMS);
        sockaddr_in client_addr;

        socklen_t len = sizeof client_addr;
        int client_fd;
        while (1)
        {
            int nfds = epoll_wait(epoll_fd, events, NUMS,-1);

            for (int i = 0; i < nfds; ++i)
            {
                if(ev.data.fd == service_fd && events[i].events&EPOLLIN) //接收到数据，读socket
                {
                    client_fd = accept(service_fd,(struct sockaddr*)&client_addr, &len);
                    if (client_fd==-1 && errno == EINTR)
                        continue;

                    setSockNonBlock(client_fd);
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
                else if(events[i].events&EPOLLIN)
                {
                    // 读三位，前四个表示数据类型，后16位表示数据长度，中间空4位。
                    int n = read(events->data.fd, index,3);
                    //n = index[1]<<8+index[2];
                    n = atoi(index+1);

                    buf = (char *)malloc(n);
                    if (!buf)
                        perror("malloc error");
                    char *buf_ptr = buf;
                    while ((n = read(events->data.fd, buf_ptr, 4096))!=0)
                    {
                        if (n==-1)
                        {
                            if(errno == EINTR)
                                continue;
                            else if (errno ==EAGAIN)
                                break;
                            else
                                std::cerr<<" read error : "<<errno;
                        }
                        buf_ptr+=n;
                    }
                    dealData(buf,buf_ptr-buf);

                } else
                {
                    //
                }
            }
        }
    }
}
/*
struct sigaction sig;
sigemptyset(&sig.sa_mask);
sig.sa_flags = 0;
sig.sa_handler = fun;
sigaction(SIGCHLD,&sig,0);



 else if(events[i].events&EPOLLOUT) //有数据待发送，写socket
                {
                     struct myepoll_data* md = (myepoll_data*)events[i].data.ptr;    //取数据
                     sockfd = md->fd;
                     send( sockfd, md->ptr, strlen((char*)md->ptr), 0 );        //发送数据
                     ev.data.fd=sockfd;
                     ev.events=EPOLLIN|EPOLLET;
                     epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); //修改标识符，等待下一个循环时接收数据
                }
*/
