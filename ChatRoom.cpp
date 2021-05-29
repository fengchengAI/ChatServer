//
// Created by feng on 2021/5/13.
//

#include "ChatRoom.h"
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "utils.h"
#include "config.h"
void message_body::decode(u_char *messagehead_r) {
    length = static_cast<u_int32_t >(messagehead_r[5] + (messagehead_r[4]<<8) + (messagehead_r[3]<<16) + (messagehead_r[2]<<24));
    //body.options  = static_cast<u_int8_t >(messagehead_r[1]);
    type = static_cast<TYPE>(messagehead_r[0] & 0x0f);
    head = static_cast<u_int8_t >(messagehead_r[0]) >>4;
}
std::pair<u_char *, size_t> message_body::encode() {

    size_t lengthdata = 26+length;
    if (!receiver_.empty()) lengthdata+=20;
    // TODO if (body.message_id!=0) length+=4;
    u_char *returndata = (u_char *)calloc(lengthdata, 1);
    returndata[0] = type + (VERSION<<4);
    returndata[1] = 0;
    returndata[2] = length & 0xff000000 ;
    returndata[3] = length & 0x00ff0000 ;
    returndata[4] = length & 0x0000ff00 ;
    returndata[5] = length & 0x000000ff ;

    //memcpy(data, messagehead_w,6);
    memcpy(returndata+6, sender_.c_str(), sender_.length());
    if (!receiver_.empty())
        memcpy(returndata+26,receiver_.c_str(),receiver_.length());
    /* TODO
    if (!body.message_id)
    {
        data[length-4-body.length] = body.message_id & 0xff000000;
        data[length-3-body.length] = body.message_id & 0x00ff0000;
        data[length-2-body.length] = body.message_id & 0x0000ff00;
        data[length-1-body.length] = body.message_id & 0x000000ff;
    }
    */

    memcpy(returndata+lengthdata-length, data, length);
    free(data);
    data = returndata+lengthdata-length;

    return {returndata, lengthdata};
}

ChatRoom::ChatRoom(std::string receiver_, std::string sender_, bool is_group_):
            receiver(receiver_), sender(sender_), is_group(is_group_)
            {
                std::string path = std::string(get_current_dir_name());
                path.append("/chatlog/").append(sender_).append("-").append(receiver_).append(".log");
                int fd = open(path.c_str(), O_CREAT|O_RDWR|O_APPEND, S_IRUSR | S_IWUSR);
                close(fd);
                fs.open(path, std::ios_base::in|std::ios_base::out|std::ios::app);
            }
void ChatRoom::init() {
    // TODO 获取本地记录
    load();
}

void ChatRoom::load() {
    if (fs.is_open()){
        std::string line;
        if(fs.fail()){
            std::cerr<<"error oprening file myname!"<<std::endl;
            exit(-1);
        }
        while(getline(fs,line))
            std::cout<<line<<std::endl;
        fs.clear();
        fs.seekp(ios::beg);
    }else{
        std::cout<<"log文本打开失败"<<std::endl;
    }
}

void ChatRoom::insert(std::string str) {

    if (fs.is_open()){
        if(fs.fail()){
            std::cerr<<"error oprening file myname!"<<std::endl;
        }
        fs<<str;
        fs.flush();
    }else{
        std::cout<<"log文本打开失败"<<std::endl;
    }
}
