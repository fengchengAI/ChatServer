//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_CLIENT_H
#define UNIX_NETWORK_CLIENT_H

#include <deque>
#include <sys/epoll.h>
#include <mutex>
#include <condition_variable>

#include "config.h"
#include "Account.h"
#include "ChatRoom.h"

using std::string;

struct Announcement{ // 这个实现不太好
    TYPE type;
    u_char* data;
    // int id;
    // int num;  //如果是发送的文本消息，这里是数据数量
//TODO 是否需要发送方名字
    string send_name;
    Announcement(TYPE t, u_char* d):type(t),data(d){}
};
class Client {
private:
    //std::thread ui_thread;
    int status;  //{0表示初始状态， 1表示正在登陆， 2表示正在注册， 3表示登陆成功}
    // 如果服务器判断登陆正确，就发送DATA数据，负责发送NO

    int port;
    struct epoll_event ev, events[2];
    int epoll_fd;
    std::string username;
    std::string nowchatwith;
    std::mutex mymutex;
    std::condition_variable cn;
    std::string rootpath;
    std::string ip;

    int client_fd;
    u_char messagehead_r[6];
    u_char messagehead_w[6];  // 写数据用到的

    std::deque<std::pair<int, string> >  responsebuf;
    std::deque<message_body> messagebuf;  //这里是消息池，每个要发送的信息都会暂存在这里

    std::unordered_map<std::string, std::deque<message_body>> messagebuf_recv;    
    //这个是接受的buff，如果我和a聊天，b给我发消息了，就存在这里
    
    
    std::unordered_map<int, message_body > command; //第一个int表示命令的id，
    std::unordered_map<std::string, ChatRoom*> activate_room;

    std::unordered_map<std::string, Announcement *>notice;

    std::deque<std::pair<std::string, Announcement *>>new_notice;


    std::mutex lock;    // 针对messagebuf加锁，副线程中对messagebuf增加push，主线程中对messagebuf 进行pop
    std::mutex etmutex; // 针对epoll加锁，在副线程中对服务器对epoll添加out权限，在主线程中取消out权限
    Account account;

public:
    std::string selectRoom();

    void chatwith(std::string room_,std::vector<std::string> &name, bool isgroup);
    ~Client();
    Client(const string &, int);
    void destroy();
    std::string makeRoom();

    void makeFriend();
    void do_sign_in_or_up();
    void Sign_in(string name, string passwad);
    void Sign_up(string name, string passwad);
    void chatwith(std::string name, bool isgroup);
    void changechat(std::string);
    void home();
    void parseAccount(char *data, int length);
    int parse(std::string command, std::string filter = string());
    bool response();

//处理消息
    void deal_mes(int id);
    void ui();
    void showfriend();
    void do_sent(message_body body);
    void do_recv(message_body const &);
    void push(message_body data);
    message_body pop();
    bool init();
    void run();
    void loadrecvbuf();  // 如果接受的消息的发送方不是正在聊天的对话，消息就不会显示，而是放在messagebuf_recv中，直到进入到这个对话框中

    std::string show(const message_body &);

};


#endif //UNIX_NETWORK_CLIENT_H
