cmake_minimum_required(VERSION 3.15.0)

project(rabbitmq-qt VERSION 1.0.0 DESCRIPTION "Работа с брокером сообщений")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(rabbitmq-c CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)

enable_testing()
add_subdirectory(test)
