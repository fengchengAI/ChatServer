//
// Created by feng on 2021/3/5.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <csignal>
#include <sys/wait.h>
#include <vector>
#include <cerrno>
#include <sys/epoll.h>
#include "Server.h"
#include "utils.h"
#include "config.h"
#include <memory>
using namespace std;


void Server::do_recv(std::string name, char *data, u_int32_t length, TYPE type)
{
    if (type == TYPE::TEXT)
    {
        std::cout.write(data,length);
        std::fflush(0);
    }
    std::cout<<"reve from "<<name<< " "<<length<<"字节数据"<<std::endl;
}
void Server::do_sent(std::string name, char * data, u_int32_t length, TYPE type)
{

}


bool Server::init()
{

    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(MYPORT);
    service_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    extern int errno;
    servicefd = socket(AF_INET,SOCK_STREAM,0);
    if (!servicefd)
    {
        std::cerr<<"socket 创建失败 :"<<errno <<strerror(errno);
    }

    if (bind(servicefd, (struct sockaddr *)&service_addr, sizeof service_addr))
    {
        std::cerr<<"bind error :"<<errno <<strerror(errno);
    }
    if (listen(servicefd, NUMS))
    {
        std::cerr<<"listen error :"<<errno <<strerror(errno);
    }

    epoll_fd = epoll_create(NUMS);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    /* TODO 服务器需要设置SO_REUSEADDR吗
    int opt = 1;
    setsockopt(servicefd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    */
    setSockNonBlock(servicefd);

    ev.data.fd = servicefd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, servicefd, &ev);

    return true;

}

std::pair<std::shared_ptr<char>, u_int32_t> Server::pop(int fd) {
    std::pair<std::shared_ptr<char>, u_int32_t> temp = messagebuf[id2name[fd]].front();
    messagebuf[id2name[fd]].pop_front();
    return temp;
}

void Server::push(int fd, std::pair<std::shared_ptr<char>, u_int32_t> data) {
     messagebuf[id2name[fd]].push_back(data);
}

void Server::encode(u_int8_t version, u_int8_t type, u_int8_t users, u_int32_t datalength)
{
    messagehead_w[0] = type + (version<<4);
    messagehead_w[1] = users;
    messagehead_w[2] = datalength & 0xff000000 ;
    messagehead_w[3] = datalength & 0x00ff0000 ;
    messagehead_w[4] = datalength & 0x0000ff00 ;
    messagehead_w[5] = datalength & 0x000000ff ;
}
void Server::decode(u_int8_t &version, u_int8_t &type, u_int8_t &users, u_int32_t &datalength) {

    datalength = static_cast<u_int32_t >(messagehead_r[5] + (messagehead_r[4]<<8) + (messagehead_r[3]<<16) + (messagehead_r[2]<<24));
    users  = static_cast<u_int8_t >(messagehead_r[1]);
    type = static_cast<u_int8_t>(messagehead_r[0] & 0x0f);
    version = static_cast<u_int8_t >(messagehead_r[0] & 0xf0);
}
Server::Server()
{
}

void Server::run()
{
    while (servicefd>0)
    {

        int nfds = epoll_wait(epoll_fd, events, NUMS ,-1);
        for (int i = 0; i < nfds; ++i)
        {
            if(clientfds.count(events[i].data.fd) && events[i].events& EPOLLOUT)  // 可写

            {
                while (!messagebuf[id2name[events[i].data.fd]].empty())
                {
                    std::pair<std::shared_ptr<char>, u_int32_t> data = pop(events[i].data.fd);
                    // TODO 什么时候free（data.first）

                    char* buf= data.first.get();

                    bzero(buf+ 26,20);
                    memcpy(buf+26 ,id2name[events[i].data.fd].c_str(),id2name[events[i].data.fd].length());
                    writedata(events[i].data.fd, buf, data.second);
                }
                ev.events =  EPOLLIN | EPOLLET; //取消epoll的write监视
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev);
            }
            if(clientfds.count(events[i].data.fd)&& events[i].events & EPOLLIN) //可读

            {

                u_int8_t type;
                u_int8_t version;
                u_int8_t users;
                u_int32_t datalength;
                int n; // 这里很重要只要读取到头消息，就一定要读取完整，阻塞读取直到读完一个完整的数据。即在tou中指定的size
                while ((n = read(events[i].data.fd, messagehead_r, 6))>0)
                {
                    readdata(events[i].data.fd, messagehead_r+n, 6-n);
                    decode(version,type,users,datalength);
                    if (version==VERSION && type == CONNECT && users == 0&& datalength ==0)  // 客户端第一次发送消息

                    {
                        std::cout<<"new client connecting to service "<<std::endl;
                        char buf[20];
                        readdata(events[i].data.fd, buf, 20);
                        id2name[events[i].data.fd] = chartostring(buf,20);
                        name2id[chartostring(buf,20)] = events[i].data.fd;
                        char* shakedata = (char *)calloc(46,1);
                        // 虽然实际上这里只需要6个字节的头文件。但是这里这样做后，就是空间换时间，不用再到write中再调用if判断是不是一个头文件了

                        encode(0, 6, 0,0);
                        memcpy(shakedata,messagehead_w,6);
                        memcpy(shakedata+6,buf,20);
                        push(events[i].data.fd,{std::shared_ptr<char >(shakedata), 6});
                        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                    }
                    else if (VERSION!=version)  // 不是协议消息
                    {
                        cerr<<"Protocol Mismatch";
                        break;
                    }
                    else  // 正常聊天消息
                    {
                        databuf_r = (char *)(malloc(46+datalength));
                        readdata(events[i].data.fd, databuf_r + 6, 20);
                        memcpy( databuf_r, messagehead_r, 6);
                        std::vector<std::string> names(users);
                        char *namebuf = static_cast<char *>(calloc(users * 20, 1));
                        for (int j = 0; j < users; ++j) {
                            names.at(j) = chartostring(namebuf+j*20,20);
                        }
                        free(namebuf);
                        readdata(events[i].data.fd, databuf_r+46, datalength);
                        std::shared_ptr<char> ptr(databuf_r);
                        for (std::string str: names)
                        {
                            if (name2id.count(str))
                            {
                                ev.events = ev.events | EPOLLOUT;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, name2id[str], &ev);
                                push(name2id[str],{ ptr, datalength+46});
                            } else
                            {
                                //TODO 离线消息
                            }
                        }
                    }
                }
            }
            if (servicefd == events[i].data.fd && events[i].events& EPOLLIN)  // 新的连接,客户端connect

            {
                int conn_sock;
                sockaddr_in remote;
                socklen_t remotelen;
                while ((conn_sock = accept(servicefd, (struct sockaddr *) &remote, &remotelen)) > 0) {
                    setSockNonBlock(conn_sock);
                    ev.data.fd = conn_sock;
                    ev.events = EPOLLIN | EPOLLET;
                    clientfds.insert(conn_sock);
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev);
                }
                if (conn_sock == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
                        perror("accept");}
            }
        }
    }
}

void Server::destroy()
{
    close(servicefd);
    for(int fd : clientfds)
    {
        close(fd);
    }
}

Server::~Server()
{
    destroy();
}

void handle(int)
{
    std::cout<<"Server now exit"<<std::endl;
    static Server service;
    service.destroy();
}
int main()
{
    Server service;

    /*
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = &handle;
    sigaction(SIGTTIN,&sig,0);
    */
    service.init();
    service.run();
}




