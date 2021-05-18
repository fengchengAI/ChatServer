//
// Created by feng on 2021/3/6.
//

#ifndef UNIX_NETWORK_ACCOUNT_H
#define UNIX_NETWORK_ACCOUNT_H

#include <string>
#include <unordered_map>
#include <functional>
/*
 * show list;
 * select user;
 * "open a new chat group windows"
 *  sent message
 *  另一个进程一直在读数据。
 *
 *
*/
#include <mysqlx/xdevapi.h>
#include <string>
using mysqlx::Table;

#include "Sql.h"
#include "ChatRoom.h"

using mysqlx::Table;
class Sql;

// user  数据库有几个字段，
class Account {
private:
    std::string name;
    std::size_t password;
    Sql sql;
    bool gender;
    Sql *sql_ptr;
    std::vector<std::string> friends;
    std::vector<std::string> rooms;
    std::map<std::string, std::vector<std::string>> roomsmembers;
    std::unordered_map<std::string, ChatRoom*> activate_room;
public:

    std::string makeFriend();
    std::string makeRoom();
    bool response(std::string data);
    void init();
    bool Sign_up(std::string name_, std::string password_,bool gender_);
    bool Sign_in(std::string name, std::string password_);
    std::string getName();

    explicit Account(Sql* s_ptr):sql_ptr(s_ptr){};
    ~Account();


};

#endif //UNIX_NETWORK_ACCOUNT_H
