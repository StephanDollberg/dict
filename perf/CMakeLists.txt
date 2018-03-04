include_directories(${TEST_SOURCE_DIR}/..)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_executable(perf_test perf_test.cpp)
target_link_libraries(perf_test benchmark pthread)
