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
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include "Client.h"
#include "utils.h"
#include "config.h"
using ::Client;

void ::Client::do_recv(std::string name, char const *data, u_int32_t length, TYPE type)
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
    service_addr.sin_addr.s_addr =inet_addr(ip.c_str());
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

    char *shakedata = (char *)calloc(26, sizeof (char) );
    encode(0,5,0,0);
    // 这里是第一次连接，会使用固定的头信息。然后加上自己的用户名发送给服务器。
    // 服务器收到这个消息后，就会将对应客户端的fd和这个名字进行绑定（map）

    memcpy(shakedata, messagehead_w,6);
    memcpy(shakedata+6,username.c_str(),username.length());

    push(std::make_pair(shakedata, 26));
    return true;

}


void Client::ui()
{
    // 这里模拟的是UI发送窗口。
    home();
    bool flag;
    while (flag)
    {

        std::cout<<"输入数据类型\n :eg TEXT 1;"<<std::endl;
        switch (fgetc(stdin)-48)
        {
            case TYPE::TEXT:
            {

                std::string sentname = getName() ;
                std::cout<<"请输入不超过200字符的文本："<<std::endl;
                std::pair<const char *, size_t> message_body = getData();
                encode(12, 1, TYPE::TEXT, message_body.second);
                char * message = generateData(sentname, message_body.first, message_body.second, -1);
                push(std::make_pair(message, 46+message_body.second));
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
                break;
            }
            case TYPE::FRIEND:
            {
                std::cout<<"请输入好友用户名"<<std::endl;
                string name = getName() ;
                int id = getRandValue();
                command[id] = std::pair<TYPE, string>{TYPE::FRIEND, name};
                encode(12, 1, TYPE::FRIEND, 4);
                char * message = generateData(name, nullptr, 4, id);
                push(std::make_pair(message, 50));
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
                break;
            }
            case TYPE::ROOM:
            {
                std::cout<<"请输入群员信息，成员之间以‘，’分割，以换行结束"<<std::endl;
                string name = getName();
                int id = getRandValue();
                command[id] = std::pair<TYPE, string>{TYPE::ROOM, name};
                encode(12, 1, TYPE::ROOM, name.length());
                char * message = generateData(string(), name.c_str(), name.length()+4, id);
                push(std::make_pair(message, 50+name.length()));
                std::unique_lock<std::mutex> eplock(etmutex);
                ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                eplock.unlock();
                break;
            }
            default:
            {
                std::cout<<"数据类型错误"<<std::endl;
                break;
            }
        }
    }
}

std::pair<char const*, u_int32_t> Client::pop() {
    std::lock_guard<std::mutex> lck(lock);
    std::pair<char const *, u_int32_t> temp = messagebuf.front();
    messagebuf.pop_front();
    return temp;
}

void Client::push(std::pair<char const *, u_int32_t> data) {
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
                    std::pair<char const*, u_int32_t> data = pop();
                    writedata(events[i].data.fd, data.first, data.second);
                    free(const_cast<char *>(data.first));
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

char *Client::generateData(string name, const char *buf, size_t length, int message_id) {
    char *ptr = (char *) calloc(46+length, 1);
    memcpy(ptr, messagehead_w, 6);
    memcpy(ptr+6, username.c_str(), 20);
    memcpy(ptr+26, name.c_str(), 20);
    if (message_id==-1)
        memcpy(ptr+46, buf, length);
    else{
        ptr[46] = message_id & 0xff000000 ;
        ptr[47] = message_id & 0x00ff0000 ;
        ptr[48] = message_id & 0x0000ff00 ;
        ptr[49] = message_id & 0x000000ff ;
        memcpy(ptr+50, buf, length-4);
    }
    return ptr;
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

    std::cin>>data;
    if ()
    {
        /* code */
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
    std::vector<std::string> room = account.getfriends();
    if (std::find(room.begin(), room.end(), name)!= room.end())
    {
        if ()
        {
            /* code */
        }
        
    }
    
}
std::string Client::makeFriend(){
    string data;
    std::cout<<"请输入添加好友的名称"<<std::endl;
    std::cin>>data;
    return data;
}
std::string Client::makeRoom(){

}
bool Client::parse(std::string command, std::string filter) {
    // 对command的解析，其中command的命令限制在filter开头。
    // show contact, do 0, d0 1, deal, select;
    if (filter.find(command)!=std::string::npos)
    {
        if (command == "show contact")
        {
            showfriend();
        }else if(command == "do 0"){
            //添加好友
        }else if(command == "do 1"){
            //添加群聊
        }else if(command.find("deal")!=std::string::npos){
            // 对这个请求应答
        }else if(command.find("select")!=std::string::npos){
            string name = string(command.find("select")+7+command.begin(), command.end());
            changechat(name);
        }  
    }else 
        return 0;
}
