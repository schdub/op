cmake_minimum_required(VERSION 2.4)

project(optests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")
set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR})

find_package(Threads REQUIRED)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
set(gtest_disable_pthreads ON CACHE INTERNAL "")

add_executable(optests tests.cpp)

target_link_libraries(optests ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
