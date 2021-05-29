//
// Created by feng on 2021/3/15.
//

#ifndef UNIX_NETWORK_SERVER_H
#define UNIX_NETWORK_SERVER_H

#include <deque>
#include <mutex>
#include <set>
#include <map>
#include <memory>
#include <vector>
#include "Sql.h"
#include "config.h"
#include "ChatRoom.h"

#define NUMS 100
class Server
{
private:
    struct epoll_event events[NUMS];
    int epoll_fd;
    int servicefd;
    Sql *sql_ptr;
    std::set<int> clientfds;   
    std::map<int, std::string> id2name;
    std::map<std::string, int> name2id;

    u_char messagehead_r[6];
    u_char messagehead_w[6];  // 写数据用到的

    std::map<int, std::deque<message_body>> messagebuf;


public:
    Server();
    ~Server();

    void do_recv(std::string name, u_char *data, u_int32_t length, TYPE type);
    void do_sent(std::string name, u_char * data, u_int32_t length, TYPE type);
    message_body pop(int);
    void push(int id, message_body data);
    bool init();
    void insertOffHistory(message_body);
    void loadOffHistory(std::string);
    void run();
    void destroy();
};
#endif //UNIX_NETWORK_SERVER_H
