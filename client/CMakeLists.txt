cmake_minimum_required(VERSION 3.22.1)
project(HotRing_Client)

set(CMAKE_CXX_STANDARD 17)

add_executable(HotRing_Client main.cpp
        Client.h)