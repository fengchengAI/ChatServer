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
struct Announcement{
    int type;
    string data;
    int id;
    int num;
    Announcement(int t, string d, int i, int n):type(t), data(d), id(i), num(n){}
};
class Client {
private:
    struct epoll_event ev, events[2];
    int epoll_fd;
    std::string username;
    std::string nowchatwith;

    std::string rootpath;
    std::string ip;
    int client_fd;
    u_char messagehead_r[6];
    u_char messagehead_w[6];  // 写数据用到的

    std::deque<std::pair<int, string> >  responsebuf;
    std::deque<message_body> messagebuf;  //这里是消息池，每个要发送的信息都会暂存在这里
    std::unordered_map<std::string, std::deque<message_body>> messagebuf_recv;
    std::unordered_map<int, message_body > command; //第一个int表示命令的id，
    std::unordered_map<std::string, ChatRoom*> activate_room;

    std::unordered_map<std::string, Announcement *>notice;


    std::mutex lock;    // 针对messagebuf加锁，副线程中对messagebuf增加push，主线程中对messagebuf 进行pop
    std::mutex etmutex; // 针对epoll加锁，在副线程中对服务器对epoll添加out权限，在主线程中取消out权限
    Account account;

public:
    void destory();
    ~Client();
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
