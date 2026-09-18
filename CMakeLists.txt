cmake_minimum_required(VERSION 3.15.0)
cmake_policy(SET CMP0016 NEW)

project(rabbitmq-qt VERSION 1.0.0 DESCRIPTION "Работа с брокером сообщений")

find_package(rabbitmq-c CONFIG QUIET)
find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(${CMAKE_SOURCE_DIR})
# add_subdirectory(src/protocol)
add_executable(test_consumer
                src/server/main.cpp
                src/server/Server.cpp
                src/server/Worker.cpp)

add_executable(test_producer
                src/client/main.cpp
                src/client/Client.cpp)
# add_subdirectory(src/client)

target_link_libraries(test_consumer PRIVATE rabbitmq::rabbitmq
                                            Qt5::Core)
target_link_libraries(test_producer PRIVATE rabbitmq::rabbitmq)

enable_testing()
# add_subdirectory(test/server)
# add_subdirectory(test/client)
