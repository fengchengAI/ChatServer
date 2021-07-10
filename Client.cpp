//
// Created by feng on 2021/3/11.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <sys/stat.h>
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
#include <string>
#include <algorithm>
#include <thread>
#include <sys/stat.h>

#include "Client.h"
#include "utils.h"
#include "config.h"
AES_KEY decrypt_key;
AES_KEY encrypt_key;

void ShowCerts (SSL* ssl)
{
    X509 *cert;
    char *line;

    cert=SSL_get_peer_certificate(ssl);
    if(cert !=NULL){
        printf("数字证书信息：\n");
        line=X509_NAME_oneline(X509_get_subject_name(cert),0,0);
        printf("证书：%s\n",line);
        free(line);
        line=X509_NAME_oneline(X509_get_issuer_name(cert),0,0);
        printf("颁发者：%s\n",line);
        free(line);
        X509_free(cert);
    }else
        printf("无证书信息！\n");
}


//消息存入本地缓存区
void Client::do_sent(message_body body) {
    if (body.type == TYPE::TEXT){
        activate_room[body.receiver_]->insert(show(body));
    }else if (body.type == TYPE::FRIEND){

    }else if(body.type == TYPE::ROOM){

    } else{

    }
}

void Client::do_recv(message_body const & body)
{   //因为建立群的命令和发送文本的命令
    if (body.type == TYPE::TEXT){

//! 当前发消息的是正在聊天的
        if(body.is_group){
            std::vector<string> roombers = account.getroomsmembers(nowchatwith);
            for(string members:roombers) {
                if(members == body.sender_){
                    string showdata = show(body);
                    cout<<showdata<<endl;
                    activate_room[nowchatwith]->insert(showdata);
                }
            }
        }
        else if (nowchatwith==body.sender_){
            string showdata = show(body);
            cout<<showdata<<endl;
            activate_room[body.sender_]->insert(showdata);

        }
//不是正在聊的，就放入缓冲区messagebuf_recv
//notice是存放以上的消息，如果存在直接加1，不存在，就new一个。

        else{
            messagebuf_recv[body.sender_].push_back(body); 
            Announcement* new_Annocement = new Announcement(TYPE::TEXT,body.data);
            new_notice.push_back(pair<string,Announcement*>(body.sender_,new_Annocement));
        }

    }else if (body.type == TYPE::FRIEND) {

        messagebuf_recv[body.sender_].push_back(body);
        Announcement *new_Annocement = new Announcement(TYPE::FRIEND, body.data);
        new_notice.push_back(pair<string, Announcement *>(body.sender_, new_Annocement));

    }else if(body.type == TYPE::ROOM){
        messagebuf_recv[body.sender_].push_back(body);
        Announcement* new_Annocement = new Announcement(TYPE::ROOM,body.data);
        new_notice.push_back(pair<string,Announcement*>(body.sender_,new_Annocement));
    
    }else if (body.type == TYPE::VIDEO){
        rootpath.append("0");
        int fd = open(rootpath.c_str(), O_CREAT|O_RDWR|O_APPEND, S_IRUSR | S_IWUSR);
        if (fd>0){
            writedata(fd, body.data, body.length);
            free(body.data);
            cout<<rootpath<<"  :recv a file"<<endl;
        }else {
            cout<<"write file error"<<endl;
        }
    } else{

    }
}

bool Client::init()
{
    rootpath = std::string(get_current_dir_name());
    string files = rootpath.append("/files");
    if (0 != access(files.c_str(), 0))
    {
        int isCreate = mkdir(files.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if( !isCreate )
            cout<<"create path:"<<files<<endl;
        else
            cerr<<"create path failed!"<<endl;
    }
    //存储聊天消息(本地存储)
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
    //! ip地址需要指定
    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(port);
    service_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    extern int errno;
    client_fd = socket(AF_INET,SOCK_STREAM,0);
    int opt = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

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
    epoll_fd = epoll_create(1);
    if (epoll_fd==-1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    init_ssl();
    if (!handshake(client_fd)) return false;

    //实现端口复用

    setSockNonBlock(client_fd);
    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

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
    std::unique_lock<std::mutex> lck(lock);
    messagebuf.push_back(data);
    lck.unlock();

    struct epoll_event ev;
    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;

    std::unique_lock<std::mutex> eplock(etmutex);
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    eplock.unlock();
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
                    std::pair<u_char *, size_t> encodemessage = data.encode(&encrypt_key);
                    writedata(events[i].data.fd, encodemessage.first, encodemessage.second);
                    cout<<"主线程客户端发送一条消息"<<endl;
                    do_sent(data);
                    free(encodemessage.first);
                }
                
                struct epoll_event ev;
                ev.data.fd = client_fd;
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
                    body.decode_head(messagehead_r);

                    if (VERSION!=body.head)  // 不是协议消息
                    {
                        cerr<<"客户端 Protocol Mismatch";
                        break;
                    }
                    else  // 正常聊天消息
                    {
                        if (body.type == TYPE::YES)
                        {
                            if (status == 1)  // 实际上不会执行这里，因为登陆的回应是DATA，注册的回应是YES
                            {
                                cout<<"登陆成功，正在加载数据"<<endl;
                                //cn.notify_one();
                            }else if(status == 2){
                                cout<<"注册成功，正在加载数据"<<endl;
                                status = 3;
                                cn.notify_one();
                            }
                        }else if (body.type == TYPE::NO)
                        {
                            if (status == 1)
                            {
                                cout<<"登陆失败"<<endl;
                                status = 0;
                                cn.notify_one();
                            }else if(status == 2){
                                cout<<"注册失败"<<endl;
                                status = 0;
                                cn.notify_one();
                            }
                        }else if (body.type == TYPE::DATA)
                        {
                            u_char sendname[20];
                            readdata(client_fd, sendname, 20);
                            username = string((char*)sendname);

                            u_char myname[20];
                            readdata(client_fd, myname, 20);
                            u_char *readdatabuf = (u_char *)calloc(body.length, 1);
                            readdata(client_fd, readdatabuf, body.length);

                            parseAccount((char *)readdatabuf, body.length);
                            cn.notify_one();

                        }else{
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
                                // 处理消息
                                do_recv(body);
                            }
                        }
                    }    
                }    
            }
        }
    }    
}

void Client::parseAccount(char *data, int length){
                
    vector<string>roommembers;
    string roomname;        
    int left = 8;
    for (int i = 8; i < length; ++i) {
        if (data[i]=='}')
        {
            left = i+1;
            break;
        }
        else if (data[i]==','){
            
            account.addFriend(string(data+left, i-left));
            left = i+1;
        }
    }
    for (int i = left; i < length; ++i) {
        if (data[i]=='{')
        {
            roomname = string(data+left, i-left);
            left = i+1;
        }
        else if (data[i]=='}')
        {
            account.addRoom(roomname, roommembers);
            roommembers.clear();
            left = i+1;
        }
        else if (data[i]==','){
            roommembers.push_back(string(data+left, i-left));
            left = i+1;
        }
    }
    status = 3;
}

void Client::home() {
    // 返回聊天主页面
    // 此时可以添加好友，建立群消息，也可以应答命令。
    // 可以选择聊天对象。
    sleep(2);
    cout<<" ---------------------------------------------\n";
    cout<<"|                                             |\n";
    cout<<"|              请选择你要需要的功能：             |\n";
    cout<<"|              show contact:查看联系人          |\n";
    cout<<"|              do 0:发起单独聊天                |\n";
    cout<<"|              do 1:发起群聊                    |\n";
    cout<<"|              deal n:n代表以上消息对应的序号     |\n";
    cout<<"|              select x:和x聊天                |\n";
    cout<<"|              exit:退出                       |\n";
    cout<<" ---------------------------------------------\n\n";
    while (1)
    {
        int num = new_notice.size();
        map<std::string, int>TEXT_count;
        std::cout << "当前一共有：" << num <<" 条消息要处理" << std::endl;
        for(int i=0;i<num;i++)
        {
            std::cout << "第" << i+1 << "条消息为：";
            if(new_notice[i].second->type == FRIEND)
            {
                std::cout << new_notice[i].first<<" 关于添加好友的处理"<<endl;
            }
            if(new_notice[i].second->type == ROOM)
            {
                std::cout << new_notice[i].first<<" 关于添加群聊的处理"<<endl;
            }
            if(new_notice[i].second->type == TEXT)
            {
                if(!TEXT_count.count(new_notice[i].first))
                {
                    TEXT_count[new_notice[i].first] = 1;
                }
                else
                {
                    TEXT_count[new_notice[i].first] ++;
                }
                std::cout << new_notice[i].first << " 发来的第" << TEXT_count[new_notice[i].first]
                 << "条聊天消息" << std::endl;
            }
        } 

        char data[1024];
        cin.sync();
        cin.ignore(std::numeric_limits<streamsize>::max(),'\n');
        cout << "请输入你选择的功能" << endl;
        cin.getline(data,1024);
        int id = parse(data);
        if(-1!=id)
        {
            //处理的是消息,删除处理好的消息
            new_notice.erase(new_notice.begin()+id-1);
        }
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
    nowchatwith = name;  // TODO 当离开对话框，应该对这个清空。
    chatwith(name, is_group);

}


void Client::makeFriend(){
    string reciver_name;
    std::cout<<"请输入添加好友的名称" <<std::endl;
    std::cin>>reciver_name;

    //初始化一个消息体
    message_body body;
    body.type = TYPE::FRIEND;
    body.sender_ = username;
    body.receiver_ = reciver_name;
    body.head = VERSION;

    //放入messagebuf中
    push(body);
    
}

std::string Client::makeRoom(){

    string room_name;
    std::cout << "请输入要建立的群聊名字" << std::endl;
    std::cin >> room_name;

    string member;
    std::cout<<"请输入邀请的群聊成员名字之间以，间隔"<<std::endl;
    std::cin>>member;

    message_body m;
    m.type = TYPE::ROOM;
    m.sender_ = username;
    m.receiver_ = room_name;
    u_char *m_data = (u_char*) calloc(member.length()+1,1);
    memcpy(m_data, member.c_str(),member.length()+1);
    m.data = m_data;
    m.length = member.length()+1;
    m.head = VERSION;
    push(m);

    return member;
}

bool Client::response() {

    std::cout<<"是否接受该请求，确认请按1，拒绝请按0"<<std::endl;
    string s;
    while(1){
        std::cin>>s;
        if (s.size()==1&&s.front()=='1')
            return 1;
        else if(s.size()==1&&s.front()=='0')
            return 0;
    }
    //TODO
}

//! 两个参数 只传入一个
int Client::parse(std::string command, std::string filter) {
    // 对command的解析，其中command的命令限制在filter开头。
    // show contact, do 0, d0 1, deal, select;
    //if (filter.find(command)!=std::string::npos)
    //{
        if (command == "exit")
        {
            destroy();
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
            // 解析消息标号
            string s_id;
            s_id = string(command.find("deal")+5+command.begin(),command.end());
            cout << "消息标号为：" << s_id << endl;
            int id = std::stoi(s_id);
            deal_mes(id);
            return id;
        }
        else if(command.find("select")!=std::string::npos){
            string name = string(command.find("select")+7+command.begin(), command.end());
            changechat(name);
        }
            
        return -1;
        
}

void Client::deal_mes(int id)
{
    // 解析id对应的消息体
    Announcement* announce = new_notice[id-1].second;
    std::string send_name = new_notice[id-1].first;
    TYPE type = announce->type;
    u_char* data = announce->data;

    // 主要分为四种应答（对方同意加好友/加群，对方不同意加好友/加群，自己同意加好友/加群，自己不同意加好友/加群）
    message_body body;
    //主要处理回复对方的加好友/加群请求
    bool if_agree;
    if(type==FRIEND)
    {
        cout << send_name << "请求添加好友" << endl;
        if_agree = response();
        body.is_group = false;
    }
    else if(type==ROOM)
    {
        cout << send_name << "请求加入群聊,群聊名字为：" << string((char*)data) << endl;
        if_agree = response();
        body.data = data;
        body.length = string((char*)data).length();
        body.is_group = true;
    }

    //根据回复的请求，构建消息体
    if(if_agree==true)
    {
        //同意添加
        body.type = TYPE::YES;
    }
    else
    {
        body.type = TYPE::NO;
    }

    body.head = VERSION;
    body.sender_ = username;
    body.receiver_ = send_name;

    push(body);

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
    std::string receivers{""};
    if(isgroup){
        std::vector<string> roombers = account.getroomsmembers(name);
        for(std::string str:roombers){
            if(str != username) receivers += (str+",");
        } 
    }
    string data;
//TODO data本来为指令，在输入文本时变为数据
    std::cout<<"请输入不超过200字符的文本："<<std::endl;
    while (getline(cin, data))
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
            //string filename;
            cin>>data;
            message_body body;
            int file_fd = open(data.c_str(), O_RDONLY);
            if (file_fd<=0)
            {
                cout<<data<<"文件打开错误"<<endl;
                break;
            }
            
            struct stat buf;
            fstat(file_fd, &buf);
            
            u_char *bodydata = (u_char *) calloc(buf.st_size,1);
            readdata(file_fd, bodydata, buf.st_size);

            body.data = bodydata;
            body.length = buf.st_size;

            body.type = TYPE::VIDEO;
            body.sender_ = username;
            body.receiver_ = name;
            body.is_group = isgroup;

            push(body);

        }else{

            message_body body;
            u_char *bodydata;
            if(isgroup) {
                bodydata = (u_char *) calloc(receivers.length()+data.length()+1,1);
                memcpy(bodydata, receivers.c_str(), receivers.length()+1);
                body.length = receivers.length()+data.length()+1;
            }
            else {
                bodydata = (u_char *) calloc(data.length()+1,1);
                body.length = data.length()+1;
            }
            memcpy(bodydata+receivers.length(), data.c_str(), data.length()+1);
            
            body.data = bodydata;
            body.type = TYPE::TEXT;
            body.sender_ = username;
            body.receiver_ = name;
            body.is_group = isgroup;

            push(body);
            
        }
    }
}

void Client::do_sign_in_or_up(){
    //请输入登陆/注册
    system("clear");
    std::cout << "您正在执行登陆/注册操作：" << endl;
    std::cout << "0：代表登陆，1：代表注册" << endl;
    string opt;
    std::cin >> opt;
    
    if(opt=="0")
    {
        std::string name;
        std::cout << "请输入登陆账户：";
        std::cin >> name;

        std::string password;
        std::cout << "请输入登陆密码：";
        std::cin >> password;
        password = std::to_string(std::hash<std::string>{}(password));
        Sign_in(name, password);
        status = 1;
        cout<<"正在登陆中..."<<endl;
        std::unique_lock <std::mutex> lk(mymutex);
        cn.wait(lk);
        
        if (status == 3)
        {
            ui();
        }else if (status == 0){
            do_sign_in_or_up();
        }
    }
    else if(opt=="1"){
        std::string name;
        std::cout << "请输入注册账户：";
        std::cin >> name;

        std::string password;
        std::cout << "请输入注册密码：";
        std::cin >> password;
        password = std::to_string(std::hash<std::string>{}(password));
        Sign_up(name, password);
        status = 2;
        cout<<"正在注册中..."<<endl;
        std::unique_lock <std::mutex> lk(mymutex);
        cn.wait(lk);
        if (status == 3)
        {
            ui();
        }else{
            system("clear");
            do_sign_in_or_up();
        }
    }else
    {
        std::cout << "非法操作" << endl;
        do_sign_in_or_up();
    }
}

void Client::Sign_in(string name, string passwad){
    message_body body;
    body.type = TYPE::Sign_in;
    body.head = VERSION;
    body.length = 20;
    u_char * datatemp = (u_char * )calloc(20,1);
    body.sender_ = name;
    body.receiver_ = name;
    memcpy(datatemp, passwad.c_str(), std::min((int)passwad.size(), 20));
    body.data = datatemp;
    push(body);
}
void Client::Sign_up(string name, string passwad){
    message_body body;
    body.type = TYPE::Sign_up;
    body.head = VERSION;
    body.length = 20;
    u_char * datatemp = (u_char * )calloc(20,1);
    body.sender_ = name;
    body.receiver_ = name;
    memcpy(datatemp, passwad.c_str(), std::min((int)passwad.size(), 20));
    body.data = datatemp;

    push(body);
}

std::string Client::show(const message_body &body) {
    //print body，显示形式
    std::string str = getDateTime();
    str.append(" ");

    str.append(body.sender_);
    str.append(" :");
    if (body.type == TYPE::TEXT)
        str.append(std::string ((char *)body.data, body.length));
    str.push_back('\n');
    return str;

}

Client::~Client() {
    destroy();
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
void Client::destroy(){
    close(client_fd);

    cout<<"Bye..................."<<endl;
    exit(EXIT_SUCCESS);
}

Client::Client(const string &ip_, int port_):ip(ip_), port(port_) {

}

void Client::init_ssl() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx=SSL_CTX_new(SSLv23_client_method());

    if(ctx==NULL){
        ERR_print_errors_fp(stdout);//  将错误打印到FILE中
        exit(1);
    }
}

bool Client::handshake(int fd) {
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl,fd);
    if(SSL_connect(ssl)==-1)
        ERR_print_errors_fp(stderr);
    else{
        printf("connect with %s encryption\n",SSL_get_cipher(ssl));
        ShowCerts(ssl);
    }
    if (SSL_read(ssl,Seed,32)!=32)
        printf("handshake failure");
    else
        printf("handshake success\n");
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    AES_set_decrypt_key(Seed, 256, &decrypt_key);
    AES_set_encrypt_key(Seed, 256, &encrypt_key);
}


//主函数，完成多线程操作
int main(int argc, char ** argv)
{
    if (argc<=2)
    {
        std::cerr<<"use example : ./client 127.0.0.1 9091"<<endl;
        exit(0);
    }
    Client client(string(argv[1]), atoi(argv[2]));
    client.init();
    
    thread ui_thread([& client](){
        client.do_sign_in_or_up();
    });

    client.run();
    ui_thread.join();
}
