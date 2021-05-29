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
#include <memory>


#include "Server.h"
#include "utils.h"
#include "config.h"
#include "ChatRoom.h"
using namespace std;

using mysqlx::RowResult;
using mysqlx::Row;

void Server::do_recv(std::string name, u_char *data, u_int32_t length, TYPE type)
{
    /*
    if (type == TYPE::TEXT)
    {
        std::cout.write(data,length);
        std::fflush(0);
    }
    std::cout<<"reve from "<<name<< " "<<length<<"字节数据"<<std::endl;
    */
}
void Server::do_sent(std::string name, u_char * data, u_int32_t length, TYPE type)
{

}


bool Server::init()
{

    sql_ptr = Sql::GetInstance();
    sql_ptr->init("chat","   ","account","101.132.128.237",33060);

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
    ///* TODO 服务器需要设置SO_REUSEADDR吗
    int opt = 1;
    setsockopt(servicefd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    //*/w
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

    setSockNonBlock(servicefd);
    epoll_event ev;
    ev.data.fd = servicefd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, servicefd, &ev);

    return true;

}

message_body Server::pop(int id) {
    message_body temp = messagebuf[id].front();
    messagebuf[id].pop_front();
    return temp;
}

void Server::push(int id, message_body data) {
     messagebuf[id].push_back(data);
}

Server::Server():sql_ptr(nullptr)
{

}

void Server::run()
{
    while (servicefd>0)
    {
        std::deque<message_body> temp_messagebuf; // 这是临时的，每次epoll_wait循环都会清零

        int nfds = epoll_wait(epoll_fd, events, NUMS, -1);
        temp_messagebuf.clear();
        for (int i = 0; i < nfds; ++i)
        {
            if(clientfds.count(events[i].data.fd) && events[i].events&EPOLLOUT)  // 可写
            {
                std::cout<<"EPOLLOUT被触发"<<endl;
                while (!messagebuf[events[i].data.fd].empty())
                {
                    message_body data = pop(events[i].data.fd);
                    std::pair<u_char *, size_t> forwarddata = data.encode();
                    writedata(events[i].data.fd, forwarddata.first, forwarddata.second);
                    std::cout<<"转发一条小溪"<<endl;
                }
                epoll_event ev;
                ev.data.fd = events[i].data.fd;
                ev.events =  EPOLLIN | EPOLLET; //取消epoll的write监视
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
            }
            if(clientfds.count(events[i].data.fd)&& events[i].events & EPOLLIN) //可读
            {

                int n = 0; // 这里很重要只要读取到头消息，就一定要读取完整，阻塞读取直到读完一个完整的数据。即在头中指定的size
                bzero(messagehead_r, 6);
                while ((n = read(events[i].data.fd, messagehead_r, 6))>0)
                {
                    readdata(events[i].data.fd, messagehead_r+n, 6-n);
                    message_body body;
                    body.decode(messagehead_r);
                    if (body.head!=VERSION)
                    {
                        cerr<<"Protocol Mismatch";
                        break;
                    }
                    
                    if (body.type == CONNECT && body.length ==0)  // 客户端第一次发送消息
                    {
                        std::cout<<"new client connecting to service "<<std::endl;
                        u_char buf[20];
                        string sentername = chartostring(buf,20);
                        readdata(events[i].data.fd, buf, 20);
                        id2name[events[i].data.fd] = sentername;
                        name2id[sentername] = events[i].data.fd;
                        loadOffHistory(sentername);

                    }
                    else if (body.type == TEXT) // 正常聊天消息
                    {
                        u_char receiver_[20];
                        u_char sender_[20];
                        u_char *data = (u_char *)calloc(body.length, 1);

                        readdata(events[i].data.fd, sender_, 20);
                        readdata(events[i].data.fd, receiver_, 20);
                        readdata(events[i].data.fd, data, body.length);

                        body.data = data;
                        body.sender_ = string((char *)sender_);
                        body.receiver_ = string((char *)receiver_);

                        temp_messagebuf.push_back(body);
                        //debug code
                        // cout<<"收到"<<body.sender_<<"发送的消息"<<endl;
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
                    epoll_event ev;
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


        while (!temp_messagebuf.empty())
        {
            std::cout<<"收到一条消息"<<endl;
            if (temp_messagebuf.front().is_group)
            {
                // TODO 群聊数据
                /* code */
            }else{
                if (name2id.count(temp_messagebuf.front().receiver_))
                {
                    int id = name2id.at(temp_messagebuf.front().receiver_);
                    push(id, temp_messagebuf.front());
                    epoll_event ev;
                    ev.data.fd = id;
                    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, id, &ev);
                }else{
                    insertOffHistory(temp_messagebuf.front());
                }
            }
            temp_messagebuf.pop_front();
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

void Server::insertOffHistory(message_body body) {

    Table table = sql_ptr->getTable("history");
    table.insert("sender","receiver","type","message_id","messagebody","time")
            .values(body.sender_, body.receiver_, (u_int8_t)body.type, body.message_id, std::string ((char *)body.data, body.length),getDateTime())
            .execute();

}

void Server::loadOffHistory(std::string name) {
    Table table = sql_ptr->getTable("history");
    RowResult result = table.select("id","sender","receiver","type","message_id","messagebody","time")
            .where("receiver = :receiver_ ")
            .bind("receiver_", name)
            .execute();
    while (result.count()){
        Row r = result.fetchOne();
        message_body body;
        body.sender_ = r[1].operator string();
        body.receiver_ = r[2].operator string();
        body.type = (TYPE)r[3].operator int();
        body.message_id = r[3].operator int();
        string s = r[5].operator string();
        u_char * temp = (u_char*) calloc(s.length(), 1);
        memcpy(temp, s.c_str(), s.length());
        body.data = temp;
        body.length = s.length();
        push(name2id[name], body);
    }
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


