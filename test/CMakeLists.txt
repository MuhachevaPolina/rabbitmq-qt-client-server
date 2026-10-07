find_package(GTest REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Test)

set(CMAKE_AUTOMOC ON)

add_executable(unit_tests
    test_main.cpp
    common/test_protobuf.cpp
    common/test_config.cpp
    common/test_logger.cpp
    server/test_worker.cpp
    server/test_server.cpp
    client/test_client_worker.cpp
    client/test_settings_dialog.cpp
    ../src/server/Worker.cpp
    ../src/server/Server.cpp
    ../src/client/ClientWorker.cpp
    ../src/client/SettingsDialog.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/client
)

target_link_libraries(unit_tests PRIVATE
    common
    rabbitmq::rabbitmq
    GTest::gtest
    Qt5::Core
    Qt5::Widgets
    Qt5::Test
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
