//
// Created by feng on 2021/3/11.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <csignal>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/stat.h>
#include "Client.h"
#include "utils.h"
#include "config.h"

void Client::do_sent(message_body body) {
    if (body.type == TYPE::TEXT){
        activate_room[body.receiver_]->insert(show(body));
    }else if (body.type == TYPE::FRIEND){

    }else if(body.type == TYPE::ROOM){

    } else{

    }
}

void Client::do_recv(message_body const & body)
{
    if (body.type == TYPE::TEXT){

        if (nowchatwith==body.sender_){
            string showdata = show(body);
            cout<<showdata<<endl;
            activate_room[body.sender_]->insert(showdata);

        }else{
            messagebuf_recv[body.sender_].push_back(body);
            if(notice.count(body.sender_)&&notice[body.sender_]){
                notice[body.sender_]->id += 1;
                notice[body.sender_]->type |= TYPE::TEXT;
            } else{
                notice[body.sender_] = new  Announcement(TYPE::TEXT,"", 0,1);
            }
        }

    }else if (body.type == TYPE::FRIEND){
        messagebuf_recv[body.sender_].push_back(body);
        notice[body.sender_] = new  Announcement(TYPE::FRIEND,string((char *)body.data, body.length), body.message_id, 0);

    }else if(body.type == TYPE::ROOM){
        messagebuf_recv[body.sender_].push_back(body);
        if(notice.count(body.sender_)&&notice[body.sender_]){
            notice[body.sender_]->type |= TYPE::ROOM;
            notice[body.sender_]->id = body.message_id;
            notice[body.sender_]->data = string((char *)body.data, body.length);
        } else{
            notice[body.sender_] = new  Announcement(TYPE::ROOM,string((char *)body.data, body.length), body.message_id, 0);
        }

    } else{

    }

}



bool Client::init()
{
    rootpath = string(get_current_dir_name());
    rootpath.append("/chatlog");
    if (0 != access(rootpath.c_str(), 0))
    {
        int isCreate = mkdir(rootpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if( !isCreate )
            cout<<"create path:"<<rootpath<<endl;
        else
            cerr<<"create path failed!"<<endl;
    }

    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(MYPORT);
    service_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    extern int errno;
    client_fd = socket(AF_INET,SOCK_STREAM,0);
    if (!client_fd)
    {
        std::cerr<<"socket 创建失败";
    }
    int flag = connect(client_fd,( struct sockaddr *)&service_addr, sizeof service_addr);
    if (flag)
    {
        perror("connect失败");
        std::cerr<<errno;
        return false;
    }
    std::cout<< "connect已连接"<<std::endl;
    epoll_fd = epoll_create(2);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    setSockNonBlock(client_fd);

    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

    message_body message;
    message.sender_ = username;
    message.type = TYPE::CONNECT;
    message.head = VERSION;

    push(message);

    return true;

}


void Client::ui()
{
    // 这里模拟的是UI发送窗口。
    system("clear");
    std::cout<<"welcome back, "<<username<<endl;
    home();

}

message_body Client::pop() {
    std::lock_guard<std::mutex> lck(lock);
    message_body temp = messagebuf.front();
    messagebuf.pop_front();
    return temp;
}

void Client::push(message_body data) {
    std::lock_guard<std::mutex> lck(lock);
    messagebuf.push_back(data);
}

Client::Client(std::string ip, Account account_):ip(ip), account(account_){
    username = account_.getName();
}

void Client::run()
{
    while (client_fd>0)
    {

        int nfds = epoll_wait(epoll_fd, events, 1 ,-1);

        for (int i = 0; i < nfds; ++i)
        {
            if(events[i].data.fd == client_fd && events[i].events& EPOLLOUT)
            {
                while (!messagebuf.empty())
                {
                    message_body data = pop();
                    std::pair<u_char *, size_t> encodemessage = data.encode();
                    writedata(events[i].data.fd, encodemessage.first, encodemessage.second);
                    cout<<"发送一条消息"<<endl;
                    do_sent(data);
                    free(encodemessage.first);
                }
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events =  EPOLLIN | EPOLLET ; //取消epoll的write监视，这里是必须的，具体为什么可以自己尝试修改，看看什么问题？ （程序会一直可写，所有这里是死循环）

                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
            }
            if(events[i].data.fd == client_fd && events[i].events& EPOLLIN)
            {

                int n; // 这里很重要只要读取到头消息，就一定要读取完整，阻塞读取直到读完一个完整的数据。即在协议中指定的size
                while ((n = read(events[i].data.fd, messagehead_r, 6))>0)
                {
                    readdata(events[i].data.fd, messagehead_r+n, 6-n);
                    message_body body;
                    body.decode(messagehead_r);
                    if (body.head==0 && body.type == CONNECT && body.length ==0)  // 第一次连接
                    {
                        std::cout<<"already connected to service "<<std::endl;
                    }
                    else if (VERSION!=body.head)  // 不是协议消息
                    {
                        cerr<<"Protocol Mismatch";
                        break;
                    }
                    else  // 正常聊天消息
                    {
                        u_char sendname[20];
                        readdata(client_fd, sendname, 20);
                        u_char myname[20];
                        readdata(client_fd, myname, 20);
                        u_char *readdatabuf = (u_char *)calloc(body.length, 1);
                        readdata(client_fd, readdatabuf, body.length);

                        body.sender_ = std::string ((char *)sendname);
                        body.receiver_ = std::string ((char *)myname);
                        body.data = readdatabuf;
                        int flag = strncmp((char *)myname, username.c_str(), username.length());
                        if (flag)
                        {
                            cerr<<"read error, data not my";
                            continue;
                        } else{
                            do_recv(body);
                        }
                    }    
                }    
            }
        }
    }
}

void Client::home() {
    // 返回聊天主页面
    // 此时可以添加好友，建立群消息，也可以应答命令。
    // 可以选择聊天对象。

    for (auto m: notice)
    {
        if (!m.second) continue;
        if (m.second->type & FRIEND )
        {
            std::cout<<m.first<<"请求添加好友"<<std::endl;
        }if(m.second->type & ROOM){
            std::cout<<m.first<<"请求加入群聊："<<m.second->data<<std::endl;
        }if(m.second->type & TEXT) {
            std::cout<<m.first<<"发送"<<m.second->id<<"条消息"<<std::endl;
        }
    }
    string data;

    while (1)
    {
        cout<<" -------------------------------------------\n";
        cout<<"|                                           |\n";
        cout<<"|          请选择你要需要的功能：               |\n";
        cout<<"|              show contact:查看联系人        |\n";
        cout<<"|              do 0:发起单独聊天               |\n";
        cout<<"|              do 1:发起群聊                  |\n";
        cout<<"|              deal n:回应n号命令              |\n";
        cout<<"|              select x:和x聊天               |\n";
        cout<<"|              exit:退出                      |\n";
        cout<<" ------------------------------------------- \n\n";
        getline(cin,data);
        parse(data);
    }
}

void Client::showfriend(){

    std::vector<std::string> friends = account.getfriends();
    cout<<"好友数量："<<friends.size()<<endl;
    for (size_t i = 0; i < friends.size(); i++)
    {
        std::cout<<friends.at(i)<<std::endl;
    }

    std::vector<std::string> rooms = account.getrooms();
    cout<<"群聊数量："<<rooms.size()<<endl;
    for (size_t i = 0; i < rooms.size(); i++)
    {
        std::cout<<rooms.at(i)<<std::endl;
    }
}

void Client::changechat(std::string name) {
    // 进入与name聊天的对话框中
    // name 是一个聊天室名称（群聊），或者一个用户名（1对1）
    std::vector<std::string> friends = account.getfriends();
    std::vector<std::string> room = account.getrooms();
    bool is_group;
    if (std::find(friends.begin(), friends.end(), name)!= friends.end())
    {
        is_group = false;
    }else if (std::find(room.begin(), room.end(), name)!= room.end()){
        is_group = true;
    }else{
        std::cout<<"非法接收方"<<std::endl;
        return;
    }
    if (!activate_room.count(name)){
        ChatRoom *newroom = new ChatRoom(name, username, is_group);
        activate_room[name] = newroom;
    }
    nowchatwith = name;
    chatwith(name, is_group);

}
std::string Client::makeFriend(){
    string data;
    std::cout<<"请输入添加好友的名称"<<std::endl;
    std::cin>>data;
    message_body m;
    m.message_id = getRandValue();
    m.type = TYPE::FRIEND;
    m.sender_ = username;
    push(m);
    return data;
}
std::string Client::makeRoom(){
    string data;
    std::cout<<"请输入邀请的群聊成员名字之间以，间隔"<<std::endl;
    std::cin>>data;
    message_body m;
    m.message_id = getRandValue();
    m.type = TYPE::ROOM;
    m.sender_ = username;
    m.data = (u_char*)data.c_str();
    m.length = data.length();
    push(m);
    return data;
}


bool Client::response(int messageid) {

    //std::cout<<data<<endl<<"  "<<"是否接受该请求，确认请按1，拒绝请按0"<<std::endl;
    string s;
    while(1){
        std::cin>>s;
        if (s.size()==1&&s.front()=='1')
            return 1;
        else if(s.size()==1&&s.front()=='0')
            return -1;
    }
    //TODO
}

bool Client::parse(std::string command, std::string filter) {
    // 对command的解析，其中command的命令限制在filter开头。
    // show contact, do 0, d0 1, deal, select;
    //if (filter.find(command)!=std::string::npos)
    //{
        if (command == "exit")
        {
            destory();
        }else if (command == "show contact")
        {
            showfriend();
        }else if(command == "do 0"){
            makeFriend();
            //添加好友
        }else if(command == "do 1"){
            makeRoom();
            //添加群聊
        }else if(command.find("deal")!=std::string::npos){
            // 对这个请求应答
            int id;
            // 解析id
            response(id);
        }else if(command.find("select")!=std::string::npos){
            string name = string(command.find("select")+7+command.begin(), command.end());
            changechat(name);
        }
        return 1;

    //}else
    //    return 0;
}



void Client::chatwith(std::string name, bool isgroup) {
    system("clear");
    cout<<" 聊天室:"<<name<<endl;
    cout<<" -------------------------------------------\n";
    cout<<"|              (tips)                       |\n";
    cout<<"|          发送的消息类型：                    |\n";
    cout<<"|              1:TEXT(默认)                  |\n";
    cout<<"|              2:PICTURE                    |\n";
    cout<<"|              3:SOUND                      |\n";
    cout<<"|              4:VIDEO                      |\n";
    cout<<"|              5:EXIT                       |\n";
    cout<<" ------------------------------------------- \n\n";
    activate_room[name]->init();
    loadrecvbuf();

    string data;

    //std::cout<<"请输入不超过200字符的文本："<<std::endl;
    while (getline(cin,data))
    {
        if (data == "5"||data == "EXIT"){
            nowchatwith.clear();
            return;
        }else if(data == "2"||data == "PICTURE"){
            //break;
        }else if(data == "3"||data == "SOUND"){
            //return;
        }else if(data == "4"||data == "VIDEO"){
            //return;
        }else {

            message_body body;
            u_char *bodydata = (u_char *) calloc(data.length()+1,1);
            memcpy(bodydata, data.c_str(), data.length()+1);

            body.data = bodydata;
            body.length = data.length()+1;

            body.type = TYPE::TEXT;
            body.sender_ = username;
            body.receiver_ = name;
            body.is_group = isgroup;

            push(body);
            std::unique_lock<std::mutex> eplock(etmutex);
            ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
            eplock.unlock();
        }
    }
}

std::string Client::show(const message_body &body) {
    //print body
    std::string str = getDateTime();
    str.append(" ");

    str.append(body.sender_);
    str.append(" :");
    if (body.type == TYPE::TEXT)
        str.append(std::string ((char *)body.data, body.length));
    str.push_back('\n');
    return str;

}
void Client::destory (){
    // TODO 释放资源，关闭fd，free指针等等嘛
    cout<<"Bye..................."<<endl;
    exit(EXIT_SUCCESS);
}

Client::~Client() {
    destory();
}

void Client::loadrecvbuf() {
    if(messagebuf_recv.count(nowchatwith)){
        auto deque = messagebuf_recv.at(nowchatwith);
        while(!deque.empty()){
            string recvbufbody = show(deque.front());
            cout<<recvbufbody<<endl;
            activate_room[nowchatwith]->insert(recvbufbody);
            deque.pop_front();
        }
    }
}
