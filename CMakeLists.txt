cmake_minimum_required(VERSION 3.16)
project(RemoteBash)

set(CMAKE_C_FLAGS "-Wall -Wextra -pthread")
find_package(OpenSSL REQUIRED)
add_compile_options(-lpam -lpam_misc)
include_directories(includes)

add_library(StringFunctions SHARED src/StringFunctions.c)
add_library(Log STATIC src/Log.c)

add_executable(Server server.c)
add_executable(Client client.c)
add_executable(PSWD src/pswd.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

target_link_libraries(Server StringFunctions Log pam_misc pam OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(Client StringFunctions OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(PSWD pam_misc)
target_link_libraries(PSWD pam)

add_custom_target(run_Client_udp
        COMMAND ./Client udp)
add_custom_target(run_Client_tcp
        COMMAND ./Client taris3@192.168.1.153)

add_custom_target(run_Server_udp
        COMMAND ./Server udp)

add_custom_target(run_Server_tcp
        COMMAND ./Server tcp)
add_custom_target(run_server_daemon
        COMMAND ./Server_daemon)

add_dependencies(run_Client_udp Client)
add_dependencies(run_Client_tcp Client)
add_dependencies(run_Server_udp Server)
add_dependencies(run_Server_tcp Server)
