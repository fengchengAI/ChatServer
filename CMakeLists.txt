project(unix-network)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 14)
set(build_type debug)


find_package (Threads)

add_executable(service  service-epoll.cpp Account.cpp Account.h)

target_link_libraries (service ${CMAKE_THREAD_LIBS_INIT})
