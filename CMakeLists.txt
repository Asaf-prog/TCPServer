cmake_minimum_required(VERSION 3.26)
project(computerNetworkEx3)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(computerNetworkEx3
        computerNetworkEx3.cpp
        Http.cpp
        HTTP.h
        server.cpp
        Server.h
        "ThreadPool .h"
        ThreadPool.cpp)

target_link_libraries(computerNetworkEx3 ws2_32)


