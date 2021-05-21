//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_CLIENT_H
#define UNIX_NETWORK_CLIENT_H

#include <deque>
#include <sys/epoll.h>
#include <mutex>

#include "config.h"
#include "Account.h"
#include "ChatRoom.h"

class Client {
private:
    struct epoll_event ev, events[2];
    int epoll_fd;
    std::string username;
    std::string path;
    std::string ip;
    int client_fd;
    char messagehead_r[6];
    char messagehead_w[6];  // 写数据用到的

    std::deque<std::pair<int, string> >  responsebuf;
    std::deque<message_body> messagebuf;  //这里是消息池，每个要发送的信息都会暂存在这里
    std::map<int, message_body > command; //第一个int表示命令的id，
    std::unordered_map<std::string, ChatRoom*> activate_room;


    std::mutex lock;    // 针对messagebuf加锁，副线程中对messagebuf增加push，主线程中对messagebuf 进行pop
    std::mutex etmutex; // 针对epoll加锁，在副线程中对服务器对epoll添加out权限，在主线程中取消out权限
    Account account;

public:
    std::string makeFriend();
    std::string makeRoom();
    void chatwith(std::string name, bool isgroup);
    void changechat(std::string);
    void home();
    bool parse(std::string command, std::string filter = string());
    bool response(int messageid);

    Client(std::string service_ip, Account account);
    void ui();
    void showfriend();
    void encode(u_int8_t version_,u_int8_t namelength, u_int8_t type, u_int32_t datalengtyh);
    void decode(u_int8_t &version_,u_int8_t &namelength, u_int8_t &type, u_int32_t &datalengtyh);

    std::pair<char *, size_t> encode1(message_body);
    void decode1(u_int8_t &version_,u_int8_t &namelength, u_int8_t &type, u_int32_t &datalengtyh);

    void do_recv(std::string name, char const *data, u_int32_t length, TYPE type);
    void do_sent(std::string name, char const * data, u_int32_t length, TYPE type);
    void push(message_body data);
    message_body pop();
    bool init();
    void run();

};


#endif //UNIX_NETWORK_CLIENT_H
