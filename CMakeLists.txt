cmake_minimum_required(VERSION 3.15)
project(easylog_example)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread")
endif()

include_directories(include)

add_executable(easylog_example
        example/main.cpp
)
