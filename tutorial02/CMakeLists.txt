cmake_minimum_required (VERSION 2.6)
project (leptjson_test C)

# if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
# endif()

add_library(leptjson leptjson.c)  # 编译成一个库
add_executable(leptjson_test test.c)  # 编译为一个可执行文件
target_link_libraries(leptjson_test leptjson)  # 将leptkson链接到可执行文件中
