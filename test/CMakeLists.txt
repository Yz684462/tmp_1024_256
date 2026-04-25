cmake_minimum_required(VERSION 3.10)
project(utils_tests)

set(INCLUDE_DIRS ../include)

# 启用 RISC-V 向量扩展和 GNU 扩展
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64gcv -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64gcv -D_GNU_SOURCE")

# 手动指定 GTest 路径
set(GTEST_INCLUDE_DIR "/usr/local/include")
set(GTEST_LIBRARY "/usr/local/lib/libgtest.a")
set(GTEST_MAIN_LIBRARY "/usr/local/lib/libgtest_main.a")

# 检查库文件是否存在
if(NOT EXISTS ${GTEST_LIBRARY})
    message(FATAL_ERROR "GTest library not found at ${GTEST_LIBRARY}")
endif()

if(NOT EXISTS ${GTEST_MAIN_LIBRARY})
    message(FATAL_ERROR "GTest main library not found at ${GTEST_MAIN_LIBRARY}")
endif()

message(STATUS "Using GTest: ${GTEST_LIBRARY}")
message(STATUS "Using GTest Main: ${GTEST_MAIN_LIBRARY}")

set(DUMP_ANALYZER_SOURCES ../src/utils/dump.cpp ../src/utils/code_block.cpp)
add_executable(test_dump_analyzer ${DUMP_ANALYZER_SOURCES} test_dump_analyzer.cpp)

set(ADDR_MANAGER_SOURCES ../src/utils/addr.cpp ../src/utils/code_block.cpp)
add_executable(test_addr_manager ${ADDR_MANAGER_SOURCES} test_addr_manager.cpp)

set(CODE_BLOCK_SOURCES ../src/utils/code_block.cpp)
add_executable(test_code_block ${CODE_BLOCK_SOURCES} test_code_block.cpp)

set(MAKE_DUMP_FRAGMENTS_SOURCES ../src/utils/dump.cpp  ../src/utils/code_block.cpp  ../src/vector_translation/translation_id.cpp ../src/vector_translation/translation_shared_lib.cpp)
add_executable(test_make_dump_fragments_file ${MAKE_DUMP_FRAGMENTS_SOURCES} test_make_dump_fragments_file.cpp)

# set(VECTOR_TRANSLATION_SOURCES ../src/vector_translation/translation_id.cpp ../src/vector_translation/translation_ranges.cpp ../src/vector_translation/translation_shared_lib.cpp ../src/vector_translation/vector_context.cpp)
# add_executable(test_vector_translation ${VECTOR_TRANSLATION_SOURCES} test_vector_translation.cpp)

target_include_directories(test_dump_analyzer PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})
target_include_directories(test_addr_manager PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})
target_include_directories(test_code_block PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})
target_include_directories(test_make_dump_fragments_file PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})

# target_include_directories(test_vector_translation PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})

# 关键：同时链接 gtest_main 和 gtest
target_link_libraries(test_dump_analyzer PRIVATE ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread)
target_link_libraries(test_addr_manager PRIVATE ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread)
target_link_libraries(test_code_block PRIVATE ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread)
target_link_libraries(test_make_dump_fragments_file PRIVATE ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread)
# target_link_libraries(test_vector_translation PRIVATE ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread dl)
