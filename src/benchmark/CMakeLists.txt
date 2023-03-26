find_package(glog)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../../include)

add_executable(easylog_benchmark
        main.cpp
        )

add_test(NAME easylog_benchmark COMMAND easylog_benchmark)

if (glog_FOUND)
  message(STATUS "glog_FOUND: ${glog_FOUND}")
  target_compile_definitions(easylog_benchmark PRIVATE HAVE_GLOG)
  target_link_libraries(easylog_benchmark PRIVATE glog::glog)
endif ()

if (NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
        target_link_libraries(easylog_benchmark PRIVATE -lstdc++fs)
endif()
