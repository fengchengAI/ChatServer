//
// Created by feng on 2021/3/9.
//

#include "Sql.h"
#include <iostream>
#include <mysqlx/xdevapi.h>

using mysqlx::Session;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using mysqlx::Table;
using mysqlx::RowResult;


Sql * Sql::GetInstance()
{
    static Sql *sql = new Sql();
    return sql;
}
void Sql::init(std::string const &name_, std::string  const &password_, std::string  const & host_, int port_)
{
    name = name_;
    password = password_;
    host = host_;
    port = port_;
    sess = new Session(host, port,name, password);
}
void Sql::init(std::string  const & name_, std::string  const & password_, std::string  const & database_, std::string  const & host_, int port_)
{
    name = name_;
    password = password_;
    host = host_;
    port = port_;
    database = database_;
    sess = new Session(host, port,name, password,database);
}

void Sql::changeDatabase(std::string  const & database)
{
    std::string str = "use ";
    str.append(database);
    str.append(1,';');
    sess->sql(str).execute();
}

string Sql::getValue(std::string const & name, std::string const & table)
{
    Table employees = sess->getSchema(database).getTable(table);
    RowResult res = employees.select(name)
            .where("name e :param").execute();

}

Table Sql::getTable(std::string const &name)
{
    return sess->getSchema(database).getTable(name);
}

bool Sql::commond(std::string  const & com)
{

}



