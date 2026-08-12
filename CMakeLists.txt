cmake_minimum_required(VERSION 3.15.0)
cmake_policy(SET CMP0016 NEW)

project(rabbitmq-qt VERSION 1.0.0 DESCRIPTION "Работа с брокером сообщений")

include_directories(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src/protocol)
add_subdirectory(src/server)
add_subdirectory(src/client)

enable_testing()
add_subdirectory(test/server)
add_subdirectory(test/client)
