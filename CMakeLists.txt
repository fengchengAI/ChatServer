project(unix-network)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 11)
set(build_type debug)


include_directories(/usr/include/mysql-cppconn-8)

find_package (Threads)

add_executable(service SQL.cpp SQL.h)
target_link_libraries(service /usr/lib/x86_64-linux-gnu/libmysqlcppconn8.so)
