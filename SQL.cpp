//
// Created by feng on 2021/3/9.
//

#include "SQL.h"
#include <iostream>
#include <mysqlx/xdevapi.h>

using namespace mysqlx;
using std::cout;
using std::endl;
using std::vector;

int main(){

    SessionSettings option("47.117.161.79", 33060,"chat", "LIUxin137");
    Session sess(option); //也可使用这种方式连接

    cout <<"Done!" <<endl;
    cout <<"Session accepted, creating collection..." <<endl;

}
