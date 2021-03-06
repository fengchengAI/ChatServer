//
// Created by feng on 2021/3/6.
//

#ifndef UNIX_NETWORK_ACCOUNT_H
#define UNIX_NETWORK_ACCOUNT_H

#include <string>
#include <functional>

using namespace std;
class Account {
private:
    int id;
    std::size_t password;
    //hash_box（）；

    bool gender;
    string location;
    int number;
public:
    bool Sign_up(string name_, string password_,bool gender, string location_, int number)
    {
        password = std::hash<std::string>{}(password_);

        //password = hash_box(password_);
        // 判断合法性。
        // 更新数据库
        // 更新日志
    }
    bool Sign_in(string name, string password)
    {

        // 获取数据库
        // 判断合法性。
        // 更新日志
    }
    int getID(){
        return id;

    }


};


#endif //UNIX_NETWORK_ACCOUNT_H
