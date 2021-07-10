//
// Created by feng on 2021/3/5.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <csignal>
#include <sys/wait.h>
#include <vector>
#include <cerrno>
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>


#include "Server.h"
#include "utils.h"
#include "config.h"
#include "ChatRoom.h"
using namespace std;

static int port ;
using mysqlx::RowResult;
using mysqlx::Row;
AES_KEY decrypt_key;
AES_KEY encrypt_key;

#ifndef USE_SSL
#define USE_SSL
#endif
bool Server::init()
{

    sql_ptr = Sql::GetInstance();
    sql_ptr->init("chat","   ","account","101.132.128.237",33060);

    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(port);
    service_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    extern int errno;
    //lfd(监听套接字)
    servicefd = socket(AF_INET,SOCK_STREAM,0);
    if (!servicefd)
    {
        std::cerr<<"socket 创建失败 :"<<errno <<strerror(errno);
    }
    ///* TODO 服务器需要设置SO_REUSEADDR吗（端口复用需要在bind之前）
    int opt = 1;
    setsockopt(servicefd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    //*/w
    if (bind(servicefd, (struct sockaddr *)&service_addr, sizeof service_addr))
    {
        std::cerr<<"bind error :"<<errno <<strerror(errno);
    }
    if (listen(servicefd, NUMS))
    {
        std::cerr<<"listen error :"<<errno <<strerror(errno);
    }
    //epoll套接字
    epoll_fd = epoll_create(NUMS);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    setSockNonBlock(servicefd);
    //必须是局部变量
    epoll_event ev;
    ev.data.fd = servicefd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, servicefd, &ev);

    init_ssl();
    return true;

}

//消息池存放的是消息体（消息id,数据，长度，是否是组，头文件），没有初始化的成员（发送方，接收方名字，类型）
std::pair<u_char *, std::size_t> Server::pop(int id) {
    std::lock_guard<std::mutex> lck(lock);

    auto temp = messagebuf[id].front();
    messagebuf[id].pop_front();
    return temp;
}

//存放的时候也是用的消息体的形式
void Server::push(int id, std::pair<u_char *, std::size_t>data) {
    std::unique_lock<std::mutex> lck(lock);
    messagebuf[id].push_back(data);
    lck.unlock();

    epoll_event ev;
    ev.data.fd = id;
    ev.events =  EPOLLIN | EPOLLET | EPOLLOUT;
    std::unique_lock<std::mutex> eplock(etmutex);

    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, id, &ev);
    eplock.unlock();

}

Server::Server():sql_ptr(nullptr)
{
    pool.init();
}
/*
可写指的是可以向客户端写消息，message_buf存放的是在线消息，
服务器默认客户端是在线的，一直向客户端写消息，写完后将自己的状态改为可读。

可读指的是客户端可以将数据发送给服务器，对于这个消息，一定是要先读头文件，
通过对头文件解码判断是否一致，才继续沟通。
对于客户端发送的消息，分为两种情况，如果是第一次发送，就将发送方名字添加到name2id中，
并加载发送方的历史聊天记录；如果不是第一次发送，就进行正常操作。客户端将自己要写入
的内容都暂时先放在temp_buf中，等待后续处理。

如果是第一次连接，就将fd插入到全局变量clientfds中，并设置读事件

接下来处理客户端发送给服务器的temp_buf数据
如果消息是 
        群发
        接收方在线(name2id中存在)
        接收方离线（放入网络服务器中）

*问题关键在于：如何在接收方离线的时候，将其从name2id中取出，服务器在设置完可写后，可查看下客户端的状态，
发送一个信号给客户端，判断下客户端是否在线，不在线的话，就将消息放入服务器里面。

*/
void Server::closefd(int sockfd) {
    //移除sockfd，从epoll监听树取下该fd。
    clientfds.erase(sockfd);
    name2id.erase(id2name[sockfd]);
    id2name.erase(sockfd);
    int ret;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
    if(ret == -1) {
        perror("epoll_ctl error");
        exit(1);
    }
    close(sockfd);
    printf("clean out\n");
}
void Server::run()
{
    while (servicefd>0)
    {
        std::deque<tuple<string, u_char *, size_t>> temp_messagebuf; //这是临时的，每次epoll_wait循环都会清零, 最后判断接收方是否在线

        int nfds = epoll_wait(epoll_fd, events, NUMS, -1);
        temp_messagebuf.clear();
        for (int i = 0; i < nfds; ++i)
        {
            if(clientfds.count(events[i].data.fd) && events[i].events&EPOLLOUT)  // 可写 （向客户端写)
            {
                // std::cout<<"EPOLLOUT被触发"<<endl;
                while (!messagebuf[events[i].data.fd].empty())
                {
                    auto forwarddata = pop(events[i].data.fd);
                    //encode是massage-body的一个类成员函数，重新编码按照（头文件，发送方，接收方，数据）
                    // std::pair<u_char *, size_t> forwarddata = data.encode();
                    writedata(events[i].data.fd, forwarddata.first, forwarddata.second);
                    // std::cout<<"转发一条消息"<<endl;
                }
                epoll_event ev;
                ev.data.fd = events[i].data.fd;
                ev.events =  EPOLLIN | EPOLLET; //取消epoll的write监视
                std::unique_lock<std::mutex> eplock(etmutex);
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                eplock.unlock();
            }
            if(clientfds.count(events[i].data.fd)&& events[i].events & EPOLLIN) //可读（客户端发送给服务器）
            {
                int n = 0; // 这里很重要只要读取到头消息，就一定要读取完整，阻塞读取直到读完一个完整的数据。即在头中指定的size
                bzero(messagehead_r, 6);
                /*
                int sums = 0;
                while ((n = read(events[i].data.fd, messagehead_r, 6))>=0){
                    sums +=n;
                    for (int j = 0; j < 6; ++j) {
                        cout<<messagehead_r[j]<<"  "<< static_cast<unsigned int>(messagehead_r[j])<<endl;
                    }
                }
                continue;
                */
                //
                while ((n = read(events[i].data.fd, messagehead_r, 6))>=0)
                {
                    if (n==0)
                    {
                        closefd(events[i].data.fd);
                        break;
                    }

                    //一定要读够六个字节
                    readdata(events[i].data.fd, messagehead_r+n, 6-n);
                    message_body body;
                    body.decode_head(messagehead_r);
                    if (body.head!=VERSION)
                    {
                        
                        cerr<<"服务器 Protocol Mismatch";
                        break;
                    }

                    if (body.type == Sign_in) 
                    {

                        u_char receiver_[20];
                        u_char sender_[20];
                        u_char *data = (u_char *)calloc(body.aes_encrypt_length, 1);

                        readdata(events[i].data.fd, sender_, 20);
                        readdata(events[i].data.fd, receiver_, 20);
                        readdata(events[i].data.fd, data, body.aes_encrypt_length);
                        u_char * decryptdata = (u_char *) calloc(body.aes_encrypt_length, 1);
                        ssl_decrypt(data, decryptdata, body.aes_encrypt_length, &decrypt_key);
                        string name((char *)receiver_);

                        string password = string((char *)decryptdata, body.length);
                        body.length = 0;
                        body.sender_ = name;
                        body.receiver_ = name;
                        int tempfd =  events[i].data.fd;
                        pool.submit([this, &body, password, tempfd](){
                            this->do_sign_in(body, password, tempfd);
                        });
                    }
                    else if (body.type == Sign_up) // 正常聊天消息
                    {
                        u_char receiver_[20];
                        u_char sender_[20];
                        u_char *data = (u_char *)calloc(body.aes_encrypt_length, 1);

                        readdata(events[i].data.fd, sender_, 20);
                        readdata(events[i].data.fd, receiver_, 20);
                        readdata(events[i].data.fd, data, body.aes_encrypt_length);
                        u_char * decryptdata = (u_char *) calloc(body.aes_encrypt_length, 1);
                        ssl_decrypt(data, decryptdata, body.aes_encrypt_length, &decrypt_key);

                        string name ((char*)receiver_);
                        body.sender_ = name;
                        body.receiver_ = name;
                        body.length = 0;
                        string password((char*)decryptdata, body.length);

                        int tempfd = events[i].data.fd;

                        pool.submit([this, &body, password, tempfd](){
                            this->do_sign_up(body, password, tempfd);
                        });
                    }
                    else if (body.type == TEXT||body.type == VIDEO) // 正常聊天消息
                    {

                        u_char *data = (u_char *)calloc(body.aes_encrypt_length+46, 1);
                        memcpy(messagehead_r, data, 6);
                        readdata(events[i].data.fd, data+6, 20);
                        readdata(events[i].data.fd, data+26, 20);
                        readdata(events[i].data.fd, data+46, body.aes_encrypt_length);
                        //u_char * decryptdata = (u_char *) calloc(body.aes_encrypt_length, 1);
                        //ssl_decrypt(data, decryptdata, body.aes_encrypt_length, decrypt_key);

                        std::vector<string>receivers;

                        if(body.is_group) {
                            /* TODO
                            string content((char*)decryptdata, body.length);
                            string delim = ",";
                            receivers = split(content, delim);
                            string recvdata = receivers[receivers.size()-1];
                            receivers.erase(receivers.end()-1);
                            free(data);
                            for(int i = 0; i < receivers.size();i++){
                                data == nullptr;
                                data = (u_char *)calloc(recvdata.size(), 1);
                                body.data = data;
                                body.receiver_ = receivers[i];
                                memcpy(data,recvdata.c_str(),recvdata.size());
                                temp_messagebuf.push_back(body);
                            }
                            */
                        }
                        else {

                            temp_messagebuf.push_back({string((char *)data+26, 20), data, body.aes_encrypt_length+46});
                        }

                    }
                    else if(body.type == FRIEND) //对方添加的好友请求(头文件，发送方，接收方)
                    {
                        cout << "服务器接受好友请求" << endl;
                        u_char *data = (u_char *)calloc(46, 1);
                        memcpy(messagehead_r, data, 6);
                        readdata(events[i].data.fd, data+6, 20);
                        readdata(events[i].data.fd, data+26, 20);

                        temp_messagebuf.push_back({string((char *)data+26, 20), data, 46});

                    }
//收到回复的请求后，根据YES/NO选择更新好友列表
                    else if(body.type == YES)
                    {
                        if(body.is_group == false)
                        {
                            //更新好友列表
                            cout << "更新好友列表" << endl;
                            //读取缓冲区数据(只需要读出，不需要转发，data为空)
                            u_char receiver_[20];
                            u_char sender_[20];
                            readdata(events[i].data.fd, sender_, 20);
                            readdata(events[i].data.fd, receiver_, 20);
                            Table table = sql_ptr->getTable("friendship");
                            table.insert("name1","name2","registration_date")
                                 .values(std::string((char *)receiver_), std::string((char *)sender_) ,getDateTime())
                                 .execute();

                            table.insert("name1","name2","registration_date")
                                 .values(std::string((char *)sender_), std::string((char *)receiver_) ,getDateTime())
                                 .execute();

                        }
                        else
                        {
                            //更新群聊列表
                            cout << "更新群聊列表" << endl;
			     //读取缓冲区数据(只需要读出，不需要转发，data为群聊名字)
                            u_char receiver_[20];
                            u_char sender_[20];
                            readdata(events[i].data.fd, sender_, 20);
                            readdata(events[i].data.fd, receiver_, 20);

                            u_char *data = (u_char *)calloc(body.aes_encrypt_length, 1);
                            readdata(events[i].data.fd, data, body.aes_encrypt_length);

                            u_char * decryptdata = (u_char *) calloc(body.aes_encrypt_length, 1);
                            ssl_decrypt(data, decryptdata, body.aes_encrypt_length, &decrypt_key);

                            string sendname = string((char*)sender_);
                            string receivername = string((char*)receiver_);
                            string roomname = string((char*)decryptdata, body.length);
                            cout << "通知" << receivername <<"," << sendname 
                            << "同意加入： " << roomname << endl;

                            Table table = sql_ptr->getTable("room");
                            RowResult result = table.select("name")
                                    .where("name = :receiver_ ")
                                    .bind("receiver_", roomname)
                                    .execute();
                            if (!result.count())
                            {
                                //重新插入数据
                                table.insert("name","members","registration_date")
                                .values(roomname,receivername+","+sendname+",",getDateTime())
                                .execute();

                            }
                            else
                            {
                                //添加数据
                                RowResult result_ = table.select("members")
                                    .where("name = :receiver_ ")
                                    .bind("receiver_", roomname)
                                    .execute();
                                string temp = result_.fetchOne()[0].operator string();
                                temp += sendname;
                                temp += ",";
                        
                                table.update().set("members",temp)
                                .where("name = :t")
                                .bind("t",roomname)
                                .execute();
                            }

                            //更新申请人的群聊
                            table = sql_ptr->getTable("roomship");
                            result = table.select("name","roommembers")
                                    .where("name = :receiver_ ")
                                    .bind("receiver_", receivername)
                                    .execute();
                            if(!result.count())
                            {
                                table.insert("name","roommembers")
                                .values(receivername,roomname+",")
                                .execute();
                            }
                            else
                            {
                                //有群
                                string temp = result.fetchOne()[1].operator string();
                                if(temp.find(roomname) == string::npos)
                                {
                                    temp += roomname;
                                    temp += ",";
                                    table.update().set("roommembers",temp)
                                    .where("name = :t")
                                    .bind("t",receivername)
                                    .execute();
                                }
                                
                            }
                            //更新被邀请人的群聊
                            table = sql_ptr->getTable("roomship");
                            result = table.select("name","roommembers")
                                    .where("name = :receiver_ ")
                                    .bind("receiver_", sendname)
                                    .execute();
                            if(!result.count())
                            {
                                table.insert("name","roommembers")
                                .values(sendname,roomname+",")
                                .execute();
                            }
                            else
                            {
                                //有群
                                string temp = result.fetchOne()[1].operator string();
                                if(temp.find(roomname) == string::npos)
                                {
                                    temp += roomname;
                                    temp += ",";
                                    table.update().set("roommembers",temp)
                                    .where("name = :t")
                                    .bind("t",sendname)
                                    .execute();
                                }
                            }
                    
                        }
                    }
                    else if(body.type == NO)
                    {
                        // cout << "对方不同意你的好友请求" << endl;
                        if(body.is_group == false)
                        {
                            //更新好友列表
                            cout << "对方不同意添加好友" << endl;
                            //读取缓冲区数据(只需要读出，不需要转发，data为空)
                            u_char receiver_[20];
                            u_char sender_[20];
                            readdata(events[i].data.fd, sender_, 20);
                            readdata(events[i].data.fd, receiver_, 20);
                        }
                        else
                        {
                            //更新群聊列表
                            cout << "对方不同意加入群聊" << endl;
                            //读取缓冲区数据(只需要读出，不需要转发，data为群聊名字)
                            u_char receiver_[20];
                            u_char sender_[20];
                            readdata(events[i].data.fd, sender_, 20);
                            readdata(events[i].data.fd, receiver_, 20);

                        }
                    }
                    else if(body.type == ROOM)
                    {
                        //处理群聊添加请求(接收方为群聊名字，data为群聊成员)
                        /* TODO
                        cout << "服务器接收加群聊消息" << endl;


                        u_char receiver_[20];
                        u_char sender_[20];
                        u_char *data = (u_char *)calloc(body.length, 1);


                        readdata(events[i].data.fd, sender_, 20);
                        readdata(events[i].data.fd, receiver_, 20);
                        readdata(events[i].data.fd, data, body.length);

                        //群聊字符串拆分
                        string room_members = string((char*)data);
                        string delim = ",";
                        vector<string>res = split(room_members, delim);

                        //将群聊名字放入data中
                        string room_name = string((char*)receiver_);
                        u_char *bodydata = (u_char *) calloc(room_name.length()+1,1);
                        memcpy(bodydata, room_name.c_str(), room_name.length()+1);

                        body.data = bodydata;
                        body.length = room_name.length()+1;
                        body.sender_ = string((char*)sender_);
                        body.is_group = true;
                        body.type = ROOM;

                        for(auto name : res)
                        {
                            body.receiver_ = name;
                            temp_messagebuf.push_back(body);
                        }
                        */
                    }
                }
            }
            if (servicefd == events[i].data.fd && events[i].events& EPOLLIN)  // 新的连接,客户端connect
            {
                int conn_sock;
                sockaddr_in remote;
                socklen_t remotelen = sizeof(struct sockaddr);
                while ((conn_sock = accept(servicefd, (struct sockaddr *) &remote, &remotelen)) > 0) {
#ifdef USE_SSL
                    if(!ssl_handshake(conn_sock)) continue;
#endif
                    setSockNonBlock(conn_sock);
                    epoll_event ev;
                    ev.data.fd = conn_sock;
                    ev.events = EPOLLIN | EPOLLET;
                    clientfds.insert(conn_sock);
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev);
                }
                if (conn_sock == -1) {
                    //int aaa = errno;
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
                        perror("accept");}
            }
            if (servicefd == events[i].data.fd && events[i].events& EPOLLHUP) {
                cout<<"closed"<<endl;
            }
        }

        while (!temp_messagebuf.empty())
        {
            auto frontbody = temp_messagebuf.front();
            std::cout<<"收到一条消息"<<endl;

            if (name2id.count(std::get<0>(frontbody)))
            {
                int id = name2id.at(std::get<0>(frontbody));
                push(id, {std::get<1>(frontbody), std::get<2>(frontbody)});

            }else{
                // TODO
                //if (frontbody.type != VIDEO)
                //    insertOffHistory(frontbody);
            }
            temp_messagebuf.pop_front();
        }
        
    }
}

//字符串分割函数
vector<string> Server::split(const string& str, const string& delim)
{
    vector<string>res;
    if(""==str) return res;
    char* strs = new char[str.length()+1];
    strcpy(strs, str.c_str());

    char* d = new char(delim.length()+1);
    strcpy(d, delim.c_str());

    char* p = strtok(strs, d);
    while(p)
    {
        string s = p;
        res.push_back(s);
        p = strtok(NULL, d);
    }
    return res;
}

void Server::destroy()
{
    close(servicefd);
    for(int fd : clientfds)
    {
        close(fd);
    }
    pool.shutdown();
    SSL_CTX_free(ctx);
}

Server::~Server()
{
    destroy();
}

void Server::insertOffHistory(message_body body) {

    Table table = sql_ptr->getTable("historys");
    table.insert("sender","receiver","type","message_id","messagebody","time")
            .values(body.sender_, body.receiver_, (u_int8_t)body.type, body.message_id, std::string ((char *)body.data, body.length),getDateTime())
            .execute();

}

void Server::loadOffHistory(std::string name) {
    // 加载离线消息
    Table table = sql_ptr->getTable("historys");
    RowResult result = table.select("id","sender","receiver","type","message_id","messagebody","time")
            .where("receiver = :receiver_ ")
            .bind("receiver_", name)
            .execute();
    if (!result.count()) return;
    int id = name2id[name];
    while (result.count()){
        Row r = result.fetchOne();
        message_body body;
        body.sender_ = r[1].operator string();
        body.receiver_ = r[2].operator string();
        body.type = (TYPE)r[3].operator int();
        body.message_id = r[3].operator int();
        string s = r[5].operator string();
        u_char * temp = (u_char*) calloc(s.length(), 1);
        memcpy(temp, s.c_str(), s.length());
        body.data = temp;
        body.length = s.length();
        push(id, body.encode(&encrypt_key));
        table.remove().where("id = :id_ ")
                .bind("id_", r[0].operator int ())
                .execute();
    }

}
void Server::loadAccountInfo(std::string name){
    message_body body;
    body.type = DATA;
    body.sender_ = name;
    body.receiver_ = name;
    string s = "friends{";

    //vector<string> friends;
    vector<string> rooms;
    //std::unordered_map<string, vector<string> > roomsmembers;

    Table table = sql_ptr->getTable("friendship");

    RowResult myResult = table.select("name2")
            .where("name1 = :name_ ")
            .bind("name_", name)
            .execute();

    for (auto data : myResult.fetchAll()) {
        s.append(data[0].operator string());
        s.push_back(',');
    }
    s.push_back('}');

    table = sql_ptr->getTable("roomship");

    myResult = table.select("roommembers")
            .where("name = :name_ ")
            .bind("name_", name)
            .execute();
    if (myResult.count()){
        string name_temp = myResult.fetchOne()[0].operator string();// name_temp如果有数据，最后一个是‘，’
        int left = 0;
        for (int i = 1; i < name_temp.size(); ++i) {
            if (name_temp.at(i)==','){
                rooms.push_back(string(name_temp.begin()+left, name_temp.begin()+i));
                left = i+1;
            }
        }
    }

    table = sql_ptr->getTable("room");
    for (std::string group_name : rooms) {
        //vector<string> group_members;
        myResult = table.select("members")
                .where("name = :name_ ")
                .bind("name_", group_name)
                .execute();
        s.append(group_name);
        s.push_back('{');

        if (myResult.count()){
            string name_temp = myResult.fetchOne()[0].operator string();// name_temp如果有数据，最后一个是‘，’
            s.append(name_temp);
        }
        s.push_back('}');
    }


    u_char *tempdata = (u_char *)calloc(s.length(), 1);
    memcpy(tempdata, s.c_str(), s.length());
    body.length = s.length();
    body.data = tempdata;
    push(name2id[name], body.encode(&encrypt_key));

   
}
void Server::do_sign_in(message_body &body, string const & password, int fd){
    string name = body.sender_;
    Table table = sql_ptr->getTable("user");
    RowResult myResult = table.select("name", "password", "gender","status")
            .where("name = :name_ AND password =:password_")
            .bind("name_", name)
            .bind("password_", password)
            .execute();


    Row Rowdata;
    if (myResult.count()){
        body.type = TYPE::YES;
        std::cout<<"new client connecting to service "<<std::endl;
        
        id2name[fd] = name;
        name2id[name] = fd;
        loadAccountInfo(name);
        loadOffHistory(name);
        /* 只能有一个用户在线，不能重复登陆
        data = myResult.fetchOne();

        if (Rowdata[3].operator bool())
        {
            std::cout<<"登录失败"<<endl;
            return false;
        }
        */
    }else {
        body.type = TYPE::NO;
        push(fd, body.encode(&encrypt_key));

    }
}

void Server::do_sign_up(message_body &body, string const & password, int fd){
    string name = body.sender_;
    
    Table table = sql_ptr->getTable("user");

    RowResult myResult = table.select("name", "password", "gender","status")
            .where("name = :name_ AND password =:password_")
            .bind("name_", name)
            .bind("password_", password)
            .execute();

    Row Rowdata;
    if (myResult.count()){ // 重复注册
        body.type = TYPE::NO;
        
    }else {
        body.type = TYPE::YES;
        table.insert("name", "password", "gender", "last_lojin", "status")
                .values(name, password, 0, getDateTime(), true)
                .execute();
    }

    push(fd, body.encode(&encrypt_key));

}

void Server::init_ssl() {
    SSL_library_init();
    /* 载入所有 SSL 算法 */
    OpenSSL_add_all_algorithms();
    /* 载入所有 SSL 错误消息 */
    SSL_load_error_strings();
    /* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
    ctx = SSL_CTX_new(SSLv23_server_method());
    /* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
    if (SSL_CTX_use_certificate_file(ctx, "/home/feng/Github/CLionProjects/untitled/cacert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户私钥 */
    if (SSL_CTX_use_PrivateKey_file(ctx, "/home/feng/Github/CLionProjects/untitled/privkey.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 检查用户私钥是否正确 */
    if (!SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    for (int i = 0; i < 32; ++i) {
        Seed[i] = getRandValue();
    }
    AES_set_decrypt_key(Seed, 256, &decrypt_key);
    AES_set_encrypt_key(Seed, 256, &encrypt_key);

}

bool Server::ssl_handshake(int fd) {
    SSL *ssl;
    ssl = SSL_new(ctx);
    /* 将连接用户的 socket 加入到 SSL */
    SSL_set_fd(ssl, fd);
    /* 建立 SSL 连接 */
    if (SSL_accept(ssl) == -1) {
        perror("accept");
        close(fd);
        return false;
    }

    if (SSL_write(ssl, Seed, 32)!=32)
        return false;
    unsigned char asd[24];
    SSL_read(ssl, asd, 24);
    /* 关闭 SSL 连接 */
    SSL_shutdown(ssl);
    /* 释放 SSL */
    SSL_free(ssl);
    return true;
}


void handle(int)
{
    std::cout<<"Server now exit"<<std::endl;
    static Server service;
    service.destroy();
}
int main(int argc, char **argv)
{
    if (argc<=1)
    {
        std::cerr<<"use example : ./server 9091"<<endl;
        exit(0);
    }
    port = atoi(argv[1]);
    static Server service;
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = &handle;
    sigaction(SIGTTIN, &sig,0);
    
    service.init();
    service.run();
}


