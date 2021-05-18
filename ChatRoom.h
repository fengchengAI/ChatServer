//
// Created by feng on 2021/5/13.
//

#ifndef UNIX_NETWORK_CHATROOM_H
#define UNIX_NETWORK_CHATROOM_H

#include <string>
#include <deque>
#include <map>

#include "config.h"
struct Announcement{
    std::string sender_;
    TYPE type;  // 如果是普通消息就使用nums标记数量，如果是请求命令，就使用data数据。
    std::string data;
    int nums;
};

struct message_body{
    std::string receiver_;
    std::string sender_;
    TYPE type;
    int message_id;
    char *data;
    size_t length;
};
class ChatRoom{
public:
    void init();
private:
    // 模拟一个聊天框架。
    bool is_group;
    std::string const &receiver;  // 聊天室名称，或者接收方用户名称
    std::string const &sender;
    std::deque<message_body> messagebuf;  //这里是消息池
    ChatRoom(std::string receiver_, std::string sender_, bool is_group_ = false);//
};


#endif //UNIX_NETWORK_CHATROOM_H
