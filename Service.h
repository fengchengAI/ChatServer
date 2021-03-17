//
// Created by feng on 2021/3/15.
//

#ifndef UNIX_NETWORK_SERVICE_H
#define UNIX_NETWORK_SERVICE_H

#include <deque>
#include <mutex>
#include <set>
#include <map>
#include <memory>
#include "config.h"
#include <vector>
#define NUMS 100
class Service
{
private:
    struct epoll_event ev, events[NUMS];
    int epoll_fd;
    int servicefd;

    std::set<int> clientfds;   
    std::map<int, std::string> id2name;
    std::map<std::string, int> name2id;

    char messagehead_r[6];

    char messagehead_w[6];  // 写数据用到的
    char * databuf_r;
    char * databuf_w ;  // 写数据用到。

    std::map<std::string, std::deque<std::pair< std::shared_ptr<char> , u_int32_t>>> messagebuf;


public:
    Service();
    ~Service();
    void encode(u_int8_t version_,u_int8_t users, u_int8_t type, u_int32_t datalengtyh);
    void decode(u_int8_t &version_,u_int8_t &users, u_int8_t &type, u_int32_t &datalengtyh);
    void do_recv(std::string name, char *data, u_int32_t length, TYPE type);
    void do_sent(std::string name, char * data, u_int32_t length, TYPE type);
    std::pair<std::shared_ptr<char>, u_int32_t> pop(int fd);
    void push(int fd, std::pair<std::shared_ptr<char>, u_int32_t> data);
    bool init();
    void run();
    void destroy();
};
#endif //UNIX_NETWORK_SERVICE_H
