//
// Created by feng on 2021/3/11.
//

#ifndef UNIX_NETWORK_CONFIG_H
#define UNIX_NETWORK_CONFIG_H

#define VERSION 1

#define MYPORT 9090
#define DEFAULT_LOG_PATH

// TODO NEW Protocol
// 4位开头，4位数据类型，8位接受用户数量，32位数据大小，（最大4g文件）

enum TYPE
{

    TEXT =1,
    PICTURE,
    SOUND,
    VIDEO,
    OTHER,
    COM
};
#endif //UNIX_NETWORK_CONFIG_H
