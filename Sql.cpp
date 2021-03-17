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


Sql & Sql::GetInstance()
{
    static Sql sql;
    return sql;
}
void Sql::init(std::string name, std::string password, std::string host, int port)
{
    sess = new Session(host, port,name, password);
}
void Sql::init(std::string name, std::string password, std::string database_, std::string host, int port)
{
    sess = new Session(host, port,name, password,database_);
}

void Sql::changeDatabase(std::string database)
{
    std::string str = "use ";
    str.append(database);
    str.append(1,';');
    sess->sql(str).execute();
}

string Sql::getValue(std::string name, std::string table)
{
    Table employees = sess->getSchema(database).getTable(table);
    RowResult res = employees.select(name)
            .where("name e :param").execute();

}
Table Sql::getTable()
{
    return sess->getSchema(database).getTable(tablename);
}
Table Sql::getTable(std::string name)
{
    tablename = name;
    return sess->getSchema(database).getTable(name);
}
bool Sql::setValue(std::string name, std::string value, std::string table)
{

}
bool Sql::commond(std::string com)
{

}



