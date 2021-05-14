//
// Created by feng on 2021/3/6.
//

#include <string>
#include <thread>
#include "utils.h"
#include "Client.h"
#include "Account.h"
#include "config.h"

using mysqlx::RowResult;
using mysqlx::Row;

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
            .bind("name_", name_).bind("password_", std::to_string(password))
            .execute();

    Row data = myResult.fetchOne();
    // TODO error
    // int asd = myResult.count();

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

    init();
    // 获取数据库
    // 判断合法性。
    // 更新日志
    // 获取离线信息
}

std::string Account::getName()
{
    return name;
}

class  Sql;

Account::~Account() {
    Table table = sql_ptr->getTable("user");

    table.update().set("status", false).where("name = :name_ AND password =:password_")
            .bind("name_", name).bind("password_", std::to_string(password))
            .execute();
    sql.getSessionPtr()->close();
    std::cout<<"Bye ..."<<std::endl;
}


bool Account::response() {
    std::cout<<"是否接受该请求，确认请按1，拒绝请按0"<<std::endl;
    /*
     *
     */
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
        friends.push_back(to_string(data[0].STRING));
    }

    table = sql_ptr->getTable("roomship");

    myResult = table.select("roommembers")
            .where("name = :name_ ")
            .bind("name_", name)
            .execute();
    if (myResult.count()){
        string name_temp = to_string(myResult.fetchOne()[0].STRING);// name_temp如果有数据，最后一个是‘，’
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
            string name_temp = to_string(myResult.fetchOne()[0].STRING);// name_temp如果有数据，最后一个是‘，’
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


int main()
{
    Sql *sql = Sql::GetInstance();
    sql->init("chat","   ","account","101.132.128.237",33060);

    Account account(sql);
    //bool flag = account.Sign_up("laoguo","LIUxin137", false);
    bool flag = account.Sign_in("fengcheng","LIUxin137");
    if (flag)
    {

        Client client("127.0.0.1",account);
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