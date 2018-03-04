cmake_minimum_required (VERSION 2.6)
project (dict)

set (CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -pedantic -g -fno-omit-frame-pointer -std=c++11")

add_subdirectory(test)
add_subdirectory(perf)

enable_testing()
