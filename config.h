//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_CONFIG_H
#define UNIX_NETWORK_CONFIG_H
// extern const int version;
#define VERSION 12
#define MYPORT 9090
#define DEFAULT_LOG_PATH


// TODO NEW Protocol
// 4位开头，4位数据类型，8位选项（如，是否是群聊消息，这个字节有点大，后期应该调整），32位数据大小,20字节的发送方名字，20字节的接收方名字,
// 如果是命令，后面是32位的命令id，最后是数据体。
// 如果是一个建群的命令，20个字节的接受方名字被忽略,后面的就是所有群员名字，以'，'号间隔
// 如果是一个加好友的命令,就没有数据
enum TYPE
{

    TEXT =1,
    PICTURE,
    SOUND,
    VIDEO,
    FRIEND,  //对应添加好友请求
    ROOM,   //对应群聊请求
    YES,    // 对前两个请求的响应
    NO,     // 对前面请求的响应
    OTHER
};
#endif //UNIX_NETWORK_CONFIG_H
