project(unix-network)
cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 14)
set(build_type debug)


include_directories(/usr/include/mysql-cppconn-8)
include_directories(/usr/local/include/cpp_redis)
include_directories(/usr/local/include/tacopie)

add_definitions(-DENABLE_LOG)
find_package (Threads)
add_executable(service utils.cpp Server.cpp ChatRoom.cpp Sql.cpp Redis.cpp)
add_executable(client Account.cpp Sql.cpp utils.cpp Client.cpp Log.cpp ChatRoom.cpp)
add_executable(test test.cpp Redis.cpp)
target_link_libraries(test /usr/local/lib/libcpp_redis.a)
target_link_libraries(test /usr/local/lib/libtacopie.a)

target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(service /usr/lib/x86_64-linux-gnu/libmysqlcppconn8.so)
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(service /usr/local/lib/libcpp_redis.a)
target_link_libraries(service /usr/local/lib/libtacopie.a)

