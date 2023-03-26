cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include_directories(../../include)

add_executable(test_easylog
        test_easylog.cpp
        main.cpp
        )
add_test(NAME test_easylog COMMAND test_easylog)

if (NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
        target_link_libraries(test_easylog PRIVATE -lstdc++fs)
endif()