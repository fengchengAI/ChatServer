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

#include "Client.h"
#include "utils.h"
#include "config.h"

void Client::do_recv(std::string name, char const *data, u_int32_t length, TYPE type)
{
    if (type == TYPE::TEXT)
    {
        std::cout.write(data,length);
        std::fflush(0);
        std::cout<<"reve from "<<name<< " "<<length<<"字节数据"<<std::endl;

    }else if (type == TYPE::FRIEND)
    {
        int message_id = get4BitInt(data);
        responsebuf.push_back({message_id, string (data,length-4)});
    }else if(type == TYPE::ROOM){

    } else
    {

    }

    delete data;
}
void Client::do_sent(std::string name, char const * data, u_int32_t length, TYPE type)
{

}


bool Client::init()
{

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
    push(message);

    return true;

}


void Client::ui()
{
    // 这里模拟的是UI发送窗口。
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

// 4位开头，4位数据类型，8位接受用户数量，32位数据大小，（最大4g文件）

void Client::encode(u_int8_t version, u_int8_t type, u_int8_t options, u_int32_t datalength)
{
    messagehead_w[0] = type + (version<<4);
    messagehead_w[1] = options;
    messagehead_w[2] = datalength & 0xff000000 ;
    messagehead_w[3] = datalength & 0x00ff0000 ;
    messagehead_w[4] = datalength & 0x0000ff00 ;
    messagehead_w[5] = datalength & 0x000000ff ;
}
void Client::decode(u_int8_t &version, u_int8_t &type, u_int8_t &options, u_int32_t &datalength) {

    datalength = static_cast<u_int32_t >(messagehead_r[5] + (messagehead_r[4]<<8) + (messagehead_r[3]<<16) + (messagehead_r[2]<<24));
    options  = static_cast<u_int8_t >(messagehead_r[1]);
    type = static_cast<u_int8_t>(messagehead_r[0] & 0x0f);
    version = static_cast<u_int8_t >(messagehead_r[0] & 0xf0);
}

Client::Client(std::string ip, Account account_):ip(ip),account(account_)
{

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
                    std::pair<char *, size_t> encodemessage = encode1(data);
                    writedata(events[i].data.fd, encodemessage.first, encodemessage.second);
                    free(const_cast<char *>(encodemessage.first));
                }
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events =  EPOLLIN | EPOLLET ; //取消epoll的write监视，这里是必须的，具体为什么可以自己尝试修改，看看什么问题？ （程序会一直可写，所有这里是死循环）

                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
            }
            if(events[i].data.fd == client_fd && events[i].events& EPOLLIN)
            {

                u_int8_t type;
                u_int8_t version;
                u_int8_t users;
                u_int32_t datalength;

                int n; // 这里很重要只要读取到头消息，就一定要读取完整，阻塞读取直到读完一个完整的数据。即在协议中指定的size
                while ((n = read(events[i].data.fd, messagehead_r, 6))>0)
                {
                    readdata(events[i].data.fd, messagehead_r+n, 6-n);
                    decode(version,type,users,datalength);
                    if (version==0 && type == 6 && users == 0&& datalength ==0)  // 第一次连接
                    {
                        std::cout<<"already connected to service "<<std::endl;
                    }
                    else if (VERSION!=version)  // 不是协议消息
                    {
                        cerr<<"Protocol Mismatch";
                        break;
                    }
                    else  // 正常聊天消息
                    {
                        char sendname[20];
                        readdata(client_fd, sendname, 20);
                        char myname[20];
                        readdata(client_fd, myname, 20);
                        int flag = strncmp(myname,username.c_str(),username.length());
                        if (flag)
                        {
                            cerr<<"read error, data not my";
                            continue;
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
    // show contact, do 0, d0 1, deal 2, select li;
    for (auto n: account.getnotice())
    {
        if (n.type == FRIEND )
        {
            std::cout<<n.sender_<<"请求添加好友"<<std::endl;
        }else if(n.type == ROOM){
            std::cout<<n.sender_<<"请求加入群聊："<<n.data<<std::endl;
        }else {
            std::cout<<n.sender_<<"发送"<<n.nums<<"条消息"<<std::endl;
        }
    }
    string data;

    while (1)
    {
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
        ChatRoom *newroom = new ChatRoom(username, name, is_group);
        activate_room[name] = newroom;
    }
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
    m.data = data.c_str();
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


    /*
     *
     */
}

bool Client::parse(std::string command, std::string filter) {
    // 对command的解析，其中command的命令限制在filter开头。
    // show contact, do 0, d0 1, deal, select;
    //if (filter.find(command)!=std::string::npos)
    //{
        if (command == "show contact")
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

std::pair<char *, size_t> Client::encode1(message_body body) {
    messagehead_w[0] = body.type+ (VERSION<<4);
    messagehead_w[1] = 0;
    messagehead_w[2] = body.length & 0xff000000 ;
    messagehead_w[3] = body.length & 0x00ff0000 ;
    messagehead_w[4] = body.length & 0x0000ff00 ;
    messagehead_w[5] = body.length & 0x000000ff ;
    size_t length = 26+body.length;
    if (!body.receiver_.empty()) length+=20;
    if (!body.message_id) length+=4;
    char *data = (char *)calloc(length, sizeof (char));


    memcpy(data, messagehead_w,6);
    memcpy(data+6,body.sender_.c_str(),username.length());
    if (!body.receiver_.empty())
        memcpy(data+26,body.receiver_.c_str(),username.length());
    if (!body.message_id)
    {
        data[length-4] = body.message_id & 0xff000000;
        data[length-3] = body.message_id & 0x00ff0000;
        data[length-2] = body.message_id & 0x0000ff00;
        data[length-1] = body.message_id & 0x000000ff;
    }
    return {data, length};
}

void Client::chatwith(std::string name, bool isgroup) {
    // bool flag;
    while (1)
    {

        std::cout<<"输入数据类型\n :eg \n    TEXT 1\n"
                   "    PICTURE 2\n"
                   "    SOUND 3\n"
                   "    VIDEO 4"<<std::endl;
        string data;
        while (std::cin>>data)
        {
            if (data == "exit"||data == "EXIT"){
                return;
            }
            else if( data == "TEXT"||data =="1")
            {

                std::cout<<"请输入不超过200字符的文本："<<std::endl;
                std::string textmessage;
                std::cin>>textmessage;
                message_body body;
                body.length = textmessage.length();
                body.type = TYPE::TEXT;
                body.sender_ = username;
                body.receiver_ = name;
                body.is_group = isgroup;

                push(body);
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
                break;
            }
            else
            {
                std::cout<<"数据类型错误"<<std::endl;
                break;
            }
        }
    }
}
