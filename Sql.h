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
    std::string database;  //Schema myDb= mySession.getSchema("test");
    std::string password;
    Session *sess = nullptr;
    std::string tablename; // Table employees = db.getTable("employee");
public:

    static Sql &GetInstance();
    void init(std::string  const & name, std::string  const & password, std::string  const & host = "127.0.0.1", int port = 33060);
    void init(std::string  const & name, std::string  const & password, std::string  const & database_, std::string  const & host = "127.0.0.1", int port = 33060);
    void changeDatabase(std::string  const & database);
    std::string getValue(std::string  const & name, std::string  const & table);
    Table getTable();
    Table getTable(std::string  const & name);
    Session *getSessionPtr()
    {
        return sess;
    }
    bool setValue(std::string  const & name, std::string  const & value, std::string  const & table);
    bool commond(std::string  const & com);

    Sql()
    {

    }
    ~Sql()
    {
        sess->close();
    }
};


#endif //UNIX_NETWORK_SQL_H
