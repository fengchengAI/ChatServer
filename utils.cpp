//
// Created by feng on 2021/3/11.
//

#include <fcntl.h>
#include <sys/types.h>
#include <iostream>
#include <sys/socket.h>
#include "utils.h"
#include <unistd.h>

std::string getDateTime()
{
    std::string str;
    time_t now = time(0);
    tm *ltm = localtime(&now);
    // 输出 tm 结构的各个组成部分
    str.append(to_string(ltm->tm_year+1900));
    str.append(1,'-');
    str.append(to_string(ltm->tm_mon+1));
    str.append(1,'-');
    str.append(to_string(ltm->tm_mday));
    str.append(1,' ');
    str.append(to_string(ltm->tm_hour));
    str.append(1,':');
    str.append(to_string(ltm->tm_min));
    str.append(1,':');
    str.append(to_string(ltm->tm_sec));
    return str;
}

void setSockNonBlock(int sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }
}

std::string chartostring(char *s, size_t length)
{
    int j;
    for (j = 0; j < length && static_cast<int>(s[j])!=0; ++j )
    {
    }
    return std::string(s,j);
}

bool readdata(int fd, char *buf, int length)
{
    int n;
    while ((n = recv(fd, buf,length >= 4096 ? 4096 : length,0)) != 0 && (length > 0))
    {
        if (n==-1)
        {
            if(errno == EINTR)
                continue;
            else if (errno ==EAGAIN)
            {
                usleep(1000);
                continue;
            }
            else
            {
                std::cerr<<" write error : "<<errno;
                close(fd);
                return false;
            }
        }
        length-=n;
        buf+=n;
    }
    return true;
}

bool writedata(int fd, const char *buf, int length)
{
    int n;
    while ((n = send(fd, buf,length>=4096?4096:length,0))!=0 &&(length>0))
    {
        if (n==-1)
        {
            if(errno == EINTR)
                continue;
            else if (errno ==EAGAIN)
            {
                usleep(1000);
                continue;
            }
            else
            {
                std::cerr<<" write error : "<<errno;
                close(fd);
                return false;
            }
        }
        length-=n;
        buf+=n;
    }
}

// [left,right]
int getRandValue(int left, int right){
    unsigned seed;  // Random generator seed
    // Use the time function to get a "seed” value for srand
    seed = time(0);
    srand(seed);
    return  rand() % right + left;
}

int get4BitInt(char const *buf){
    return static_cast<u_int32_t >(buf[3] + (buf[2]<<8) + (buf[1]<<16) + (buf[0]<<24));
}