find_package(GTest REQUIRED)

add_executable(unit_tests
    main_test.cpp
    ../src/server/Worker.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
)

target_link_libraries(unit_tests PRIVATE
    common
    rabbitmq::rabbitmq
    GTest::gtest
    GTest::gtest_main
    Qt5::Core
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
