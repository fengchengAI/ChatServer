//
// Created by feng on 2021/3/6.
//

#include <string>
#include <thread>
#include "utils.h"
#include "Client.h"
#include "Account.h"
#include "config.h"
bool Account::Sign_up(std::string name_, std::string password_,bool gender_)
{

    if (name_.length()>20&&name_.length()<6)
    {
        std::cout<<"注册失败(非法用户名)"<<endl;
    }
    RowResult myResult = table.select("name")
            .where("name = :name_ ")
            .bind("name_", name_)
            .execute();
    //int asd = myResult.count();
    if (myResult.count())
    {
        std::cout<<"注册失败(用户名已存在)"<<endl;
        return false;
    }
    password = std::hash<std::string>{}(password_);
    gender = gender_;
    name = name_;

    table.insert("name", "password", "gender", "last_lojin", "status")
            .values(name_, std::to_string(password), gender, getDateTime(), true).execute();
    // 判断合法性。
    // 更新数据库
    // 更新日志

    std::cout<<"插入一条数据"<<std::endl;
    return true;
}
bool Account::Sign_in(std::string name_, std::string password_)
{

    password = std::hash<std::string>{}(password_);
    /*
    RowResult myResult = table.select("name", "password", "gender","status")
            .where("name = :name_ AND password =:password_")
            .bind("name_", name_).bind("password_", std::to_string(password))
            .execute();
    */
    RowResult myResult = table.select("name")
            .where("name = :name_ ")
            .bind("name_", name_)
            .execute();
    Row data = myResult.fetchOne();
    // TODO error
    int asd = myResult.count();

    if (!myResult.count()|| data[3].BOOL == true)
    {
        std::cout<<"登录失败"<<endl;
        return false;
    }

    std::cout<<"登录成功"<<endl;

    name = name_;
    gender = data[2].BOOL;
    //id = data[3].UINT64;
    table.update().set("status", true).where("name = :name_ AND password =:password_")
            .bind("name_", name_).bind("password_", std::to_string(password))
            .execute();
    // 获取数据库
    // 判断合法性。
    // 更新日志
    // 获取离线信息
}

std::string Account::getName()
{
    return name;
}
void Account::getCom()
{

}
class  Sql;
Account::Account(Table table) : table(table) {

}

Account::~Account() {
    table.update().set("status", false).where("name = :name_ AND password =:password_")
            .bind("name_", name).bind("password_", std::to_string(password))
            .execute();
    sql.getSessionPtr()->close();
    std::cout<<"Bye ..."<<std::endl;
}

int main()
{
    Sql &sql = Sql::GetInstance();
    sql.init("chat","LIUxin137","ACCOUNT","47.117.161.79",33060);
    Table t = sql.getTable("user");
    Account account(t);
    //bool flag = account.Sign_up("fengcheng","LIUxin137", false);
    bool flag = account.Sign_in("fengcheng","LIUxin137");
    if (flag)
    {


        ::Client client("127.0.0.1", "fengcheng");
        //::Client client("127.0.0.1", account.getName());
        if (!client.init())
        {
            exit(1);
        }
        std::thread read_thread([&client]()
                                {
                                    client.input();
                                });// 开启另一个线程主要是为了随时准备写数据（即发送数据），这里可以先叫这个线程是副线程
        client.run();  // 主线程

        read_thread.join();
    }
}