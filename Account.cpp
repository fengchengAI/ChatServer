//
// Created by feng on 2021/3/6.
//

#include <string>

#include "Account.h"


std::vector<std::string> const & Account::getfriends() const{
    return friends;
}
std::vector<std::string> const & Account::getrooms() const{
    return rooms;
}
std::vector<std::string>const & Account::getroomsmembers(std::string room)const{
    return roomsmembers.at(room);
}

Account::~Account() {
}

void Account::addFriend(std::string name){
    friends.push_back(name);
}
void Account::addRoom(std::string name, std::vector<std::string> members){
    rooms.push_back(name);
    roomsmembers[name] = members;
}