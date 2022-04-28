cmake_minimum_required(VERSION 3.16)
project(RemoteBash)

set(CMAKE_C_FLAGS "-Wall -Wextra -pthread")
include_directories(includes)

add_executable(Server server.c)
add_executable(Client client.c)

add_custom_target(run_Client_udp
        COMMAND ./Client udp)
add_custom_target(run_Client_tcp
        COMMAND ./Client tcp)

add_custom_target(run_Server_udp
        COMMAND ./Server udp)

add_custom_target(run_Server_tcp
        COMMAND ./Server tcp)

add_dependencies(run_Client_udp Client)
add_dependencies(run_Client_tcp Client)
add_dependencies(run_Server_udp Server)
add_dependencies(run_Server_tcp Server)
