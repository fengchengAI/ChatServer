project(unix-network)
cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 14)
set(build_type debug)


include_directories(/usr/include/mysql-cppconn-8)


add_definitions(-DENABLE_LOG)
find_package (Threads)
option (USE_SSL "Use OPENSSL" ON)
if (USE_SSL)
    find_package(OpenSSL REQUIRED)
    add_definitions(-D ENABLE_SSL)
endif (USE_SSL)
add_executable(service utils.cpp Server.cpp ChatRoom.cpp Sql.cpp )

add_executable(client Account.cpp utils.cpp Client.cpp Log.cpp ChatRoom.cpp)


target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(service /usr/lib/x86_64-linux-gnu/libmysqlcppconn8.so)
target_link_libraries(service ${CMAKE_THREAD_LIBS_INIT})

if (USE_SSL)
    target_link_libraries(service OpenSSL::SSL)
    target_link_libraries(client OpenSSL::SSL)
endif (USE_SSL)