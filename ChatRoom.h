//
// Created by feng on 2021/5/13.
//

#ifndef UNIX_NETWORK_CHATROOM_H
#define UNIX_NETWORK_CHATROOM_H

#include <string>
#include <deque>
#include <map>

#include "config.h"
struct message_body{
    std::string name;  // 发送方
    TYPE type;
    int message_id;
    char *data;
    size_t length;
};

class ChatRoom {
    // 模拟一个聊天框架。
    bool is_group;
    std::string const &name;  // 聊天室名称，或者接收方用户名称
    std::deque<std::pair<int, std::string> >  responsebuf;
    std::deque<std::pair<char const * , u_int32_t>> messagebuf;  //这里是消息池，每个要方法的信息都会暂存在这里
    std::map<int, std::pair<TYPE, std::string> > command;
    ChatRoom(std::string name_, bool is_group_ = false):name(name_), is_group(is_group_){}
};


#endif //UNIX_NETWORK_CHATROOM_H
