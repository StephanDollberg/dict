include_directories(${TEST_SOURCE_DIR}/..)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_executable(dict_test main.cpp dict_test.cpp)

add_executable(dict_test_cpp_17 main.cpp dict_test.cpp)
target_compile_options(dict_test_cpp_17 PUBLIC "--std=c++1z")

enable_testing()
add_test(test dict_test)
add_test(test_cpp17 dict_test_cpp_17)
