//
// Created by feng on 2021/3/6.
//

#ifndef UNIX_NETWORK_ACCOUNT_H
#define UNIX_NETWORK_ACCOUNT_H

#include <string>
#include <unordered_map>
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
#include <string>
#include <unordered_map>

#include "Sql.h"
#include "ChatRoom.h"


class Account {
private:
    std::string name;
    std::size_t password;
    bool gender;
    Sql *sql_ptr;
    std::vector<std::string> friends;
    std::vector<std::string> rooms;
    std::map<std::string, std::vector<std::string>> roomsmembers;
    std::deque<Announcement> notice;

public:

    //bool response(std::string data);
    void init();
    bool makeFriend(string name);
    bool makeRoom(string data);
    bool Sign_up(std::string name_, std::string password_, bool gender_);
    bool Sign_in(std::string name_, std::string password_);
    std::string getName();
    std::vector<std::string> const & getfriends() const;
    std::vector<std::string> const & getrooms() const;
    std::deque<Announcement> const & getnotice() const;
    void addactivate_room(std::string name);
    explicit Account(Sql* s_ptr):sql_ptr(s_ptr){};
    ~Account();


};

#endif //UNIX_NETWORK_ACCOUNT_H
