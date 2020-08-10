cmake_minimum_required(VERSION 3.16)
project(multithlib)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test 
    test.cpp
)

target_link_libraries(test
    ${GTEST_LIBRARIES}
    pthread
)
