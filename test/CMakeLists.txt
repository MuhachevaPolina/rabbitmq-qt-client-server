find_package(GTest REQUIRED)

add_executable(unit_tests
    main_test.cpp
)

target_link_libraries(unit_tests PRIVATE
    common
    GTest::gtest
    GTest::gtest_main
    Qt5::Core
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
