//
// Created by feng on 2021/3/11.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include "Client.h"
#include "utils.h"
#include "config.h"


void Client::do_recv(std::string name, char const *data, u_int32_t length, TYPE type)
{
    if (type == TYPE::TEXT)
    {
        std::cout.write(data,length);
        std::fflush(0);
    }
    delete data;
    std::cout<<"reve from "<<name<< " "<<length<<"字节数据"<<std::endl;
}
void Client::do_sent(std::string name, char const * data, u_int32_t length, TYPE type)
{

}


bool Client::init()
{

    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(MYPORT);
    service_addr.sin_addr.s_addr =inet_addr(ip.c_str());
    extern int errno;
    client_fd = socket(AF_INET,SOCK_STREAM,0);
    if (!client_fd)
    {
        std::cerr<<"socket 创建失败";
    }
    int flag = connect(client_fd,( struct sockaddr *)&service_addr, sizeof service_addr);
    if (flag)
    {
        perror("connect失败");
        std::cerr<<errno;
        return false;
    }
    std::cout<< "connect已连接"<<std::endl;
    epoll_fd = epoll_create(2);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    setSockNonBlock(client_fd);

    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

    char *shakedata = (char *)calloc(26, sizeof (char) );
    //bzero(shakedata,26);
    //memset(shakedata,0,26);
    encode(0,5,0,0);
    memcpy(shakedata, messagehead_w,6);
    memcpy(shakedata+6,username.c_str(),username.length());

    push(std::make_pair(shakedata, 26));
    return true;

}


void Client::input()
{
    while (1)
    {   std::string sentname;
        std::cout<<"请输入发送方用户名"<<std::endl;
        cin>>sentname;
        std::cin.ignore();
        std::cout<<"输入数据类型\n :eg TEXT 1;"<<std::endl;
        switch (fgetc(stdin)-48)
        {
            case 1:
            {
                std::cout<<"请输入不超过200字符的文本："<<std::endl;
                // 每次最多可以发送200个字符，加上头文件，name长度
                char * mesg = (char *)calloc(246,1);
                memcpy(mesg+6,username.c_str(),username.length());
                memcpy(mesg+26,sentname.c_str(),sentname.length());

                std::cin.ignore();
                fgets(mesg+46,200, stdin);
                encode(12, 1, 1,strlen(mesg+46));
                memcpy(mesg, messagehead_w,6);
                push(std::make_pair(mesg,46+strlen(mesg+46)));
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
                break;
            }
            default:
            {
                std::cout<<"数据类型错误"<<std::endl;
                break;
            }
        }
    }
}

std::pair<char const*, u_int32_t> Client::pop() {
    std::lock_guard<std::mutex> lck(lock);
    std::pair<char const *, u_int32_t> temp = messagebuf.front();
    messagebuf.pop_front();
    return temp;
}

void Client::push(std::pair<char const *, u_int32_t> data) {
    std::lock_guard<std::mutex> lck(lock);
    messagebuf.push_back(data);
}

// 4位开头，4位数据类型，8位接受用户数量，32位数据大小，（最大4g文件）

void Client::encode(u_int8_t version, u_int8_t type, u_int8_t users, u_int32_t datalength)
{
    messagehead_w[0] = type + (version<<4);
    messagehead_w[1] = users;
    messagehead_w[2] = datalength & 0xff000000 ;
    messagehead_w[3] = datalength & 0x00ff0000 ;
    messagehead_w[4] = datalength & 0x0000ff00 ;
    messagehead_w[5] = datalength & 0x000000ff ;
}
void Client::decode(u_int8_t &version, u_int8_t &type, u_int8_t &users, u_int32_t &datalength) {

    datalength = static_cast<u_int32_t >(messagehead_r[5] + (messagehead_r[4]<<8) + (messagehead_r[3]<<16) + (messagehead_r[2]<<24));
    users  = static_cast<u_int8_t >(messagehead_r[1]);
    type = static_cast<u_int8_t>(messagehead_r[0] & 0x0f);
    version = static_cast<u_int8_t >(messagehead_r[0] & 0xf0);
}

Client::Client(std::string ip, std::string username):ip(ip),username(username)
{
}

void Client::run()
{
    while (client_fd>0)
    {

        int nfds = epoll_wait(epoll_fd, events, 2 ,-1);

        for (int i = 0; i < nfds; ++i)
        {
            if(events[i].data.fd == client_fd && events[i].events& EPOLLOUT)
            {
                while (!messagebuf.empty())
                {
                    std::pair<char const*, u_int32_t> data = pop();
                    writedata(events[i].data.fd, data.first, data.second);
                    delete data.first;
                }
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events =  EPOLLIN | EPOLLET ; //取消epoll的write监视
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
            }
            if(events[i].data.fd == client_fd && events[i].events& EPOLLIN)
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
                    if (version==0 && type == 6 && users == 0&& datalength ==0)  // 第一次连接
                    {
                        std::cout<<"already connected to service "<<std::endl;
                    }
                    else if (VERSION!=version)  // 不是协议消息
                    {
                        cerr<<"Protocol Mismatch";
                        break;
                    }
                    else  // 正常聊天消息
                    {
                        char sendname[20];
                        readdata(client_fd, sendname, 20);
                        char myname[20];
                        readdata(client_fd, myname, 20);
                        int flag = strncmp(myname,username.c_str(),username.length());
                        if (flag)
                        {
                            cerr<<"read error, data not my";
                            continue;
                        }
                        databuf_r = (char *)(malloc(datalength));
                        readdata(events[i].data.fd, databuf_r, datalength);
                        do_recv(chartostring(sendname,20), databuf_r, datalength, static_cast<TYPE>(type));
                    }
                }
            }
        }
    }
}
