cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "/home/kemier/project/github/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(httpserver)


find_package(cJSON CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)

message("httpserserver demo")
add_executable(server server.c)
 
target_link_libraries(server PRIVATE libevent::core libevent::extra libevent::pthreads  cjson)
