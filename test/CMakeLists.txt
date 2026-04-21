cmake_minimum_required(VERSION 3.10)
project(utils_tests)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test_dump_analyzer test_dump_analyzer.cpp)
add_executable(test_addr_manager test_addr_manager.cpp)
add_executable(test_code_block test_code_block.cpp)
target_link_libraries(test_dump_analyzer PRIVATE GTest::gtest_main)
target_link_libraries(test_addr_manager PRIVATE GTest::gtest_main)
target_link_libraries(test_code_block PRIVATE GTest::gtest_main)
