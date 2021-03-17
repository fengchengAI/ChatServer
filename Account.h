//
// Created by feng on 2021/3/6.
//

#ifndef UNIX_NETWORK_ACCOUNT_H
#define UNIX_NETWORK_ACCOUNT_H

#include <string>
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

using namespace mysqlx;
#include "Sql.h"

class Sql;

// user  数据库有几个字段，
class Account {
private:
    std::string name;
    std::size_t password;
    Sql sql;
    bool gender;
    //string location;
    //int number;
    Table table;
public:
    bool Sign_up(std::string name_, std::string password_,bool gender_);
    bool Sign_in(std::string name, std::string password_);
    std::string getName();
    void getCom();
    explicit Account(Table table);
    ~Account();

};

#endif //UNIX_NETWORK_ACCOUNT_H
