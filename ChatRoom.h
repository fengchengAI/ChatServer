//
// Created by feng on 2021/5/13.
//

#ifndef UNIX_NETWORK_CHATROOM_H
#define UNIX_NETWORK_CHATROOM_H

#include <string>
#include <deque>
#include <map>
#include <vector>
#include <fstream>
#include <openssl/rsa.h>
#include <openssl/aes.h>

#include "config.h"

class message_body{
public:
        int head;
        std::string receiver_;
        std::string sender_;
        size_t aes_encrypt_length;
        TYPE type;
        int message_id;
        u_char * data;
        size_t length;
        bool is_group;
        message_body(bool is_group = false):data(nullptr),message_id(0),length(0),is_group(is_group),head(VERSION){}
        std::pair<u_char *, size_t> encode(const AES_KEY *);
        void decode_head(u_char *messagehead_r);
};
void ssl_encrypt(const unsigned char *in, unsigned char *out,
                 size_t in_length, const AES_KEY *key);
void ssl_decrypt(const unsigned char *in, unsigned char *out,
                 size_t in_length, const AES_KEY *key);
class ChatRoom{
public:
    void init();
    void insert(std::string str);
    void load();
    std::fstream fs;
    ChatRoom(std::string receiver_, std::string sender_, bool is_group_ = false);

private:
    // 模拟一个聊天框架。
    bool is_group;

    std::string const &receiver;  // 聊天室名称，或者接收方用户名称
    std::string const &sender;
    std::deque<message_body> messagebuf;  //这里是消息池

};


#endif //UNIX_NETWORK_CHATROOM_H
