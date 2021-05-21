project(unix-network)
cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 14)
set(build_type debug)


include_directories(/usr/include/mysql-cppconn-8)

add_definitions(-DENABLE_LOG)
find_package (Threads)
add_executable(service utils.cpp Server.cpp)
add_executable(client Account.cpp Sql.cpp utils.cpp Client.cpp Log.cpp ChatRoom.cpp)
target_link_libraries(client /usr/lib/x86_64-linux-gnu/libmysqlcppconn8.so)
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})