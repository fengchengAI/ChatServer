//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_CLIENT_H
#define UNIX_NETWORK_CLIENT_H
#include "config.h"
#include <deque>
#include <sys/epoll.h>
#include <mutex>
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
    char * databuf_r;
    const char * databuf_w ;  // 写数据用到。
    std::deque<std::pair<char const * , u_int32_t>> messagebuf;

    std::mutex lock;    // 针对messagebuf加锁
    std::mutex etmutex; // 针对epoll// 加锁

public:
    Client(std::string service_ip, std::string username);
    void input();
    void encode(u_int8_t version_,u_int8_t namelength, u_int8_t type, u_int32_t datalengtyh);
    void decode(u_int8_t &version_,u_int8_t &namelength, u_int8_t &type, u_int32_t &datalengtyh);
    void do_recv(std::string name, char const *data, u_int32_t length, TYPE type);
    void do_sent(std::string name, char const * data, u_int32_t length, TYPE type);
    void push(std::pair<char const *, u_int32_t> data);
    std::pair<char const *, u_int32_t>  pop();
    bool init();
    void run();

};


#endif //UNIX_NETWORK_CLIENT_H
