cmake_minimum_required(VERSION 3.10)
project(PointCloudTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找GoogleTest
find_package(GTest REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/multiview
)

# 添加测试可执行文件
add_executable(test_main multiview/test_main.cpp)
target_link_libraries(test_main GTest::gtest GTest::gtest_main)

# 添加FileLibrary测试
add_executable(test_filelibrary base/test_FileLibrary.cpp)
target_link_libraries(test_filelibrary GTest::gtest GTest::gtest_main)

# 添加测试
add_test(NAME ParserTests COMMAND test_main)
add_test(NAME FileLibraryTests COMMAND test_filelibrary)

# 启用测试
enable_testing()