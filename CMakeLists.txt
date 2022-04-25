cmake_minimum_required(VERSION 3.16)
project(RemoteBash)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")

add_executable(Bash bash.c)

add_executable(Server server.c)
add_executable(Client client.c)
