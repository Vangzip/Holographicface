#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <list>

// 包含被测试的头文件
#include "FileLibrary.h"

using namespace std;
using namespace testing;

// 测试夹具类
class FileLibraryTest : public Test {
protected:
    void SetUp() override {
        // 创建临时测试目录和文件
        testDir = "/tmp/test_file_library";
        filesystem::create_directories(testDir);
        
        // 创建测试文件
        ofstream file1(testDir + "/file1.txt");
        file1 << "Test content 1";
        file1.close();
        
        ofstream file2(testDir + "/file2.txt");
        file2 << "Test content 2";
        file2.close();
        
        // 创建子目录
        filesystem::create_directories(testDir + "/subdir");
        
        ofstream file3(testDir + "/subdir/file3.txt");
        file3 << "Test content 3";
        file3.close();
    }
    
    void TearDown() override {
        // 清理测试文件
        filesystem::remove_all(testDir);
    }
    
    string testDir;
};

// 测试用例：单例模式
TEST_F(FileLibraryTest, SingletonPattern) {
    FileLibrary* instance1 = FileLibrary::getInstance();
    FileLibrary* instance2 = FileLibrary::getInstance();
    
    EXPECT_NE(instance1, nullptr);
    EXPECT_NE(instance2, nullptr);
    EXPECT_EQ(instance1, instance2); // 应该是同一个实例
}

// 测试用例：文件存在性检查
TEST_F(FileLibraryTest, FileExistenceCheck) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 测试存在的文件
    EXPECT_TRUE(lib->isFileExists(testDir + "/file1.txt"));
    
    // 测试不存在的文件
    EXPECT_FALSE(lib->isFileExists(testDir + "/nonexistent.txt"));
    
    // 测试目录（应该返回false）
    EXPECT_FALSE(lib->isFileExists(testDir));
}

// 测试用例：目录存在性检查
TEST_F(FileLibraryTest, DirectoryExistenceCheck) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 测试存在的目录
    EXPECT_TRUE(lib->isDirExists(testDir));
    
    // 测试不存在的目录
    EXPECT_FALSE(lib->isDirExists(testDir + "/nonexistent"));
    
    // 测试文件（应该返回false）
    EXPECT_FALSE(lib->isDirExists(testDir + "/file1.txt"));
}

// 测试用例：从路径获取文件名
TEST_F(FileLibraryTest, GetFileNameFromPath) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // Unix风格路径
    EXPECT_EQ(lib->getFileNameFromPath("/path/to/file.txt"), "file.txt");
    
    // Windows风格路径
    EXPECT_EQ(lib->getFileNameFromPath("C:\\path\\to\\file.txt"), "file.txt");
    
    // 只有文件名
    EXPECT_EQ(lib->getFileNameFromPath("file.txt"), "file.txt");
    
    // 路径以分隔符结尾
    EXPECT_EQ(lib->getFileNameFromPath("/path/to/"), "");
    
    // 空路径
    EXPECT_EQ(lib->getFileNameFromPath(""), "");
}

// 测试用例：路径组合
TEST_F(FileLibraryTest, CombineFilePath) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 正常组合
    EXPECT_EQ(lib->combineFilePath("/path", "to/file.txt"), "/path/to/file.txt");
    
    // 第一部分以分隔符结尾
    EXPECT_EQ(lib->combineFilePath("/path/", "to/file.txt"), "/path/to/file.txt");
    
    // 第二部分以分隔符开头
    EXPECT_EQ(lib->combineFilePath("/path", "/to/file.txt"), "/path/to/file.txt");
    
    // 空路径组合
    EXPECT_EQ(lib->combineFilePath("", "file.txt"), "file.txt");
    EXPECT_EQ(lib->combineFilePath("/path", ""), "/path");
}

// 测试用例：获取所有子文件
TEST_F(FileLibraryTest, GetAllSubFiles) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    list<string> files;
    
    // 测试包含文件和目录
    lib->getAllSubFiles(testDir, files, true, true, true);
    EXPECT_GT(files.size(), 0);
    
    // 测试只包含文件
    files.clear();
    lib->getAllSubFiles(testDir, files, false, true, true);
    for (const auto& file : files) {
        EXPECT_TRUE(filesystem::is_regular_file(file));
    }
    
    // 测试只包含目录
    files.clear();
    lib->getAllSubFiles(testDir, files, true, false, true);
    for (const auto& file : files) {
        EXPECT_TRUE(filesystem::is_directory(file));
    }
    
    // 测试文件过滤
    files.clear();
    lib->getAllSubFiles(testDir, files, false, true, true, ".txt");
    for (const auto& file : files) {
        EXPECT_TRUE(file.find(".txt") != string::npos);
    }
}

// 测试用例：路径转换
TEST_F(FileLibraryTest, PathConversion) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // Unix转Windows
    string winPath = lib->convertToWinPath("/path/to/file.txt");
    EXPECT_EQ(winPath, "\\path\\to\\file.txt");
    
    // Windows转Unix
    string unixPath = lib->convertToLinuxPath("C:\\path\\to\\file.txt");
    EXPECT_EQ(unixPath, "C:/path/to/file.txt");
}

// 测试用例：获取当前路径
TEST_F(FileLibraryTest, GetCurrentFilePath) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    string currentPath = lib->getCurrentFilePath();
    
    EXPECT_FALSE(currentPath.empty());
    // 当前路径应该存在
    EXPECT_TRUE(filesystem::exists(currentPath));
}

// 测试用例：字符串分割
TEST_F(FileLibraryTest, SplitString) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    string testString = "apple,banana,cherry";
    vector<string> result;
    
    lib->splitString(testString, ",", result);
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "apple");
    EXPECT_EQ(result[1], "banana");
    EXPECT_EQ(result[2], "cherry");
    
    // 测试空字符串
    result.clear();
    lib->splitString("", ",", result);
    EXPECT_EQ(result.size(), 0);
    
    // 测试没有分隔符
    result.clear();
    lib->splitString("single", ",", result);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "single");
}

// 测试用例：模板函数 - 字符串转换
TEST_F(FileLibraryTest, TemplateStringConversion) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 整数转换
    int intValue = 0;
    lib->convertFromstring(intValue, "42");
    EXPECT_EQ(intValue, 42);
    
    // 浮点数转换
    float floatValue = 0.0f;
    lib->convertFromstring(floatValue, "3.14");
    EXPECT_FLOAT_EQ(floatValue, 3.14f);
    
    // 字符串转字符串
    string strValue;
    lib->convertFromstring(strValue, "hello");
    EXPECT_EQ(strValue, "hello");
    
    // 转换为字符串
    string intStr = lib->convertTostring(123);
    EXPECT_EQ(intStr, "123");
    
    string floatStr = lib->convertTostring(3.14f, 2);
    EXPECT_EQ(floatStr, "3.14");
}

// 测试用例：RGB到HSV转换
TEST_F(FileLibraryTest, RgbToHsvConversion) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 测试绿色值
    EXPECT_TRUE(lib->Rgb2Hsv(0.0f, 1.0f, 0.0f)); // 纯绿色
    
    // 测试非绿色值
    EXPECT_FALSE(lib->Rgb2Hsv(1.0f, 0.0f, 0.0f)); // 纯红色
    EXPECT_FALSE(lib->Rgb2Hsv(0.0f, 0.0f, 1.0f)); // 纯蓝色
    
    // 测试边界值
    EXPECT_FALSE(lib->Rgb2Hsv(0.0f, 0.0f, 0.0f)); // 黑色
    EXPECT_FALSE(lib->Rgb2Hsv(1.0f, 1.0f, 1.0f)); // 白色
}

// 测试用例：绿色过滤
TEST_F(FileLibraryTest, GreenFilter) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // 测试绿色过滤
    EXPECT_TRUE(lib->filterGreen(0.0f, 1.0f, 0.0f, 0.8f)); // 高绿色值
    EXPECT_FALSE(lib->filterGreen(1.0f, 0.0f, 0.0f, 0.8f)); // 低绿色值
    
    // 测试不同阈值
    EXPECT_TRUE(lib->filterGreen(0.0f, 0.9f, 0.0f, 0.5f)); // 高于阈值
    EXPECT_FALSE(lib->filterGreen(0.0f, 0.3f, 0.0f, 0.5f)); // 低于阈值
}

// 测试用例：四元数转欧拉角
TEST_F(FileLibraryTest, QuaternionToEuler) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    Quate q;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    q.w = 1.0f; // 单位四元数
    
    double roll, pitch, yaw;
    lib->toEulerAngle(q, roll, pitch, yaw);
    
    // 单位四元数应该对应零旋转
    EXPECT_NEAR(roll, 0.0, 0.001);
    EXPECT_NEAR(pitch, 0.0, 0.001);
    EXPECT_NEAR(yaw, 0.0, 0.001);
}

// 测试用例：文件复制
TEST_F(FileLibraryTest, FileCopy) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    string srcFile = testDir + "/file1.txt";
    string dstFile = testDir + "/file1_copy.txt";
    
    int result = lib->copyFile(srcFile, dstFile);
    
    EXPECT_EQ(result, 0); // 应该成功
    EXPECT_TRUE(filesystem::exists(dstFile));
    
    // 验证文件内容
    ifstream src(srcFile);
    ifstream dst(dstFile);
    
    string srcContent((istreambuf_iterator<char>(src)), istreambuf_iterator<char>());
    string dstContent((istreambuf_iterator<char>(dst)), istreambuf_iterator<char>());
    
    EXPECT_EQ(srcContent, dstContent);
}

// 测试用例：错误处理 - 复制不存在的文件
TEST_F(FileLibraryTest, FileCopyNonExistent) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    string srcFile = testDir + "/nonexistent.txt";
    string dstFile = testDir + "/copy.txt";
    
    int result = lib->copyFile(srcFile, dstFile);
    
    EXPECT_NE(result, 0); // 应该失败
    EXPECT_FALSE(filesystem::exists(dstFile));
}

// 测试用例：随机数生成
TEST_F(FileLibraryTest, RandomNumberGeneration) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    string rand1 = lib->getRand();
    string rand2 = lib->getRand();
    
    EXPECT_FALSE(rand1.empty());
    EXPECT_FALSE(rand2.empty());
    
    // 两个随机数应该不同（虽然理论上可能相同，但概率极低）
    EXPECT_NE(rand1, rand2);
}

// 测试用例：获取文件父路径
TEST_F(FileLibraryTest, GetFileParentPath) {
    FileLibrary* lib = FileLibrary::getInstance();
    
    // Unix风格路径
    EXPECT_EQ(lib->getFileParentPath("/path/to/file.txt"), "/path/to");
    
    // Windows风格路径
    EXPECT_EQ(lib->getFileParentPath("C:\\path\\to\\file.txt"), "C:\\path\\to");
    
    // 只有文件名
    EXPECT_EQ(lib->getFileParentPath("file.txt"), "");
    
    // 根目录
    EXPECT_EQ(lib->getFileParentPath("/file.txt"), "/");
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}