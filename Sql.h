//
// Created by feng on 2021/3/9.
//


#ifndef UNIX_NETWORK_SQL_H
#define UNIX_NETWORK_SQL_H

#include <string>
#include <iostream>
#include <mysqlx/xdevapi.h>
using mysqlx::Session;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using mysqlx::Table;

class Sql {
private:;
    std::string host;
    std::string name;
    int port;
    std::string database;
    std::string password;
    Session *sess;
public:

    static Sql *GetInstance();
    void init(std::string const & name, std::string  const & password, std::string const & host = "101.132.128.237", int port = 33060);
    void init(std::string const & name, std::string  const & password, std::string const & database_, std::string  const & host = "101.132.128.237", int port = 33060);
    void changeDatabase(std::string  const & database);
    Table getTable(std::string const & name);

    Session *getSessionPtr()
    {
        return sess;
    }

    Sql():sess(nullptr)
    {
    }
    ~Sql()
    {
        sess->close();
    }
};


#endif //UNIX_NETWORK_SQL_H
