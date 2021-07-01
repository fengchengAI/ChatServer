//
// Created by feng on 2021/3/6.
//

#ifndef UNIX_NETWORK_ACCOUNT_H
#define UNIX_NETWORK_ACCOUNT_H

#include <string>
#include <unordered_map>
#include <vector>

#include "ChatRoom.h"


class Account {
private:
    std::string name;
    
    std::vector<std::string> friends;
    std::vector<std::string> rooms;
    std::map<std::string, std::vector<std::string>> roomsmembers;

public:

    void addFriend(std::string name);
    void addRoom(std::string name, std::vector<std::string> members);

    std::vector<std::string> const & getfriends() const;
    std::vector<std::string> const & getrooms() const;
    std::vector<std::string>const & getroomsmembers(std::string room)const;

    ~Account();

};

#endif //UNIX_NETWORK_ACCOUNT_H
