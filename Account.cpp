//
// Created by feng on 2021/3/6.
//

#include <string>
#include <thread>
#include <mysqlx/xdevapi.h>

#include "Account.h"
#include "utils.h"
#include "Client.h"
#include "config.h"

using mysqlx::RowResult;
using mysqlx::Row;

std::deque<Announcement> const & Account::getnotice() const{
    return notice;
}
std::vector<std::string> const & Account::getfriends() const{
    return friends;
}
std::vector<std::string> const & Account::getrooms() const{
    return rooms;
}
bool Account::Sign_up(std::string name_, std::string password_,bool gender_)
{

    if (name_.length()>20&&name_.length()<6)
    {
        std::cout<<"注册失败(非法用户名)"<<endl;
    }
    Table table = sql_ptr->getTable("user");
    RowResult myResult = table.select("name")
            .where("name = :name_ ")
            .bind("name_", name_)
            .execute();
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
    // TODO
    // 判断合法性。
    // 更新数据库
    // 更新日志

    std::cout<<"插入一条数据"<<std::endl;
    return true;
}
bool Account::Sign_in(std::string name_, std::string password_)
{

    password = std::hash<std::string>{}(password_);
    Table table = sql_ptr->getTable("user");

    RowResult myResult = table.select("name", "password", "gender","status")
            .where("name = :name_ AND password =:password_")
            .bind("name_", name_)
            .bind("password_", std::to_string(password))
            .execute();

    Row data;
    if (myResult.count()){
        data = myResult.fetchOne();

        if (data[3].operator bool())
        {
            std::cout<<"登录失败"<<endl;
            return false;
        }
    }else{
        return false;
    }



    std::cout<<"登录成功"<<endl;

    name = name_;
    gender = data[2].operator bool();
    // TODO 这里为了方便注释了对账户状态和时间的更新。负责在调试阶段经常无法完成对status设置0，导致无法登录
    /*
    table.update().set("status", true).where("name = :name_ AND password =:password_")
            .bind("name_", name_).bind("password_", std::to_string(password))
            .execute();
    */
    init();
    // TODO
    // 判断合法性。
    // 更新日志
    // 获取离线信息
    return true;
}

std::string Account::getName()
{
    return name;
}


Account::~Account() {
    Table table = sql_ptr->getTable("user");

    table.update().set("status", false).where("name = :name_ AND password =:password_")
            .bind("name_", name).bind("password_", std::to_string(password))
            .execute();
    sql_ptr->getSessionPtr()->close();
    std::cout<<"Bye ..."<<std::endl;
}



void Account::init() {
    // 获取账户好友，群聊信息
    std::cout<<"登录初始化中"<<endl;
    Table table = sql_ptr->getTable("friendship");

    RowResult myResult = table.select("name2")
            .where("name1 = :name_ ")
            .bind("name_", name)
            .execute();

    for (auto data : myResult.fetchAll()) {
        friends.push_back(data[0].operator string());
    }

    table = sql_ptr->getTable("roomship");

    myResult = table.select("roommembers")
            .where("name = :name_ ")
            .bind("name_", name)
            .execute();
    if (myResult.count()){
        string name_temp = myResult.fetchOne()[0].operator string();// name_temp如果有数据，最后一个是‘，’
        int left = 0;
        for (int i = 1; i < name_temp.size(); ++i) {
            if (name_temp.at(i)==','){
                rooms.push_back(string(name_temp.begin()+left, name_temp.begin()+i));
                left = i+1;
            }
        }
    }

    table = sql_ptr->getTable("room");
    for (std::string group_name : rooms) {
        vector<string> group_members;
        myResult = table.select("members")
                .where("name = :name_ ")
                .bind("name_", group_name)
                .execute();
        if (myResult.count()){
            string name_temp = myResult.fetchOne()[0].operator string();// name_temp如果有数据，最后一个是‘，’
            int left = 0;
            for (int i = 1; i < name_temp.size(); ++i) {
                if (name_temp.at(i)==','){
                    group_members.push_back(string(name_temp.begin()+left, name_temp.begin()+i));
                    left = i+1;
                }
            }
        }
        roomsmembers[group_name] = group_members;
    }
}

bool Account::makeFriend(string name_) {

    Table table = sql_ptr->getTable("friendship");
    RowResult myResult = table.select("id")
            .where("name1 = :name1_  AND name2 = :name2")
            .bind("name1_", name)
            .bind("name1_", name_)
            .execute();
    if (myResult.count()) return true;

    table.insert("name1", "name2", "registration_date")
            .values(name, name_, getDateTime())
            .execute();
}

bool Account::makeRoom(string groupname) {  // 群聊名称

    // 对roomship的更新
    Table table = sql_ptr->getTable("roomship");
    RowResult myResult = table.select("roommembers","id")
            .where("name = :name1_ ")
            .bind("name1_", name)
            .execute();
    if (!myResult.count()){
        table.insert("name", "roommembers")
        .values(name,groupname+",")
        .execute();
    }else{
        Row row = myResult.fetchOne();
        string s = row[0].operator string();
        s.append(groupname+",");
        table.update()
        .set("roommembers", s)
        .where("id = "+ to_string(row[1].operator int()))
        .execute();
    }

    // 对room的更新
    table = sql_ptr->getTable("room");
    myResult = table.select("name","members")
            .where("name = :name1_ ")
            .bind("name1_", groupname)
            .execute();
    if (!myResult.count()){
        table.insert("name", "members", "registration_date")
                .values(groupname, name+",", getDateTime())
                .execute();
    }else{
        Row row = myResult.fetchOne();
        string s = row[1].operator string ();
        s.append(name+",");
        table.update()
                .set("members", s)
                .where("id = "+row[1].operator int())
                .execute();
    }
}


int main()
{
    Sql *sql = Sql::GetInstance();
    sql->init("chat","   ","account","101.132.128.237",33060);

    Account account(sql);
    //bool flag = account.Sign_up("fengcheng","LIUxin137", false);
    bool flag = account.Sign_in("fengcheng","LIUxin137");
    if (flag)
    {

        Client client("127.0.0.1",account);
        if (!client.init())
        {
            exit(1);
        }
        std::thread read_thread([&client]()
                                {
                                    client.ui();
                                });// 开启另一个线程主要是为了随时准备写数据（即发送数据），这里可以先叫这个线程是副线程
        client.run();  // 主线程

        read_thread.join();
    }
}
/*

CREATE TABLE `room`(
`id` INT UNSIGNED AUTO_INCREMENT,
`name` CHAR(20) NOT NULL,
`members` VARCHAR (4600) NOT NULL,
`registration_date` DATETIME NOT NULL,
 PRIMARY KEY (`id`)
);

CREATE TABLE `friendship`(
`id` INT UNSIGNED AUTO_INCREMENT,
`name1` CHAR(20) NOT NULL,
`name2` CHAR(20) NOT NULL,
`registration_date` DATETIME NOT NULL,
PRIMARY KEY (`id`)
);

*/
