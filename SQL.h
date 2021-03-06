//
// Created by feng on 2021/3/9.
//


#ifndef UNIX_NETWORK_SQL_H
#define UNIX_NETWORK_SQL_H

#include <string>
#include <iostream>
#include <mysqlx/xdevapi.h>

using namespace mysqlx;
using std::cout;
using std::endl;
using std::vector;
class SQL {
private:

    string host;
    string name;
    int port;
    string database;
    string password;
    Session *sess = nullptr;

public:
    SQL()
    {

    };
    static SQL &GetInstance()
    {
        static SQL sql;
        return sql;
    }
    void init(string name, string password, string host = "127.0.0.1", int port = 33060)
    {
        sess = new Session(host, port,name, password);
    }
    void init(string name, string password, string database_, string host = "127.0.0.1", int port = 33060)
    {
        sess = new Session(host, port,name, password,database_);
    }

    void changeDatabase(string database)
    {

    }

};


#endif //UNIX_NETWORK_SQL_H
