cmake_minimum_required(VERSION 3.31)
project(tunnel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)

add_library(tun STATIC src/shared/tun.cpp)
target_include_directories(tun PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)

add_library(io STATIC src/shared/io.cpp)
target_include_directories(io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)

add_library(utils STATIC src/shared/utils.cpp)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)

add_executable(client src/client/main.cpp)
add_executable(server src/server/main.cpp)

target_link_libraries(client PRIVATE Boost::headers tun io utils)
target_link_libraries(server PRIVATE Boost::headers tun io utils)
