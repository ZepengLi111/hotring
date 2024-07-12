cmake_minimum_required(VERSION 3.22.1)
project(HotRing)

set(CMAKE_CXX_STANDARD 17)

add_executable(HotRing_Server main.cpp
        HotRing.h
        HashTable.h
        random.cpp
        Server.h)
