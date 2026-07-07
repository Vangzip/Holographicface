#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// 模拟OSG相关头文件
#include <osg/ArgumentParser>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>

// 包含被测试的代码
#include "main.cpp"

using namespace std;
using namespace testing;

// Mock类用于模拟OSG相关功能
class MockArgumentParser {
public:
    MOCK_METHOD(int, find, (const string& name), (const));
};

class MockFileLibrary {
public:
    MOCK_METHOD(bool, isFileExists, (const string& filepath), (const));
    MOCK_METHOD(void, getAllSubFiles, (const string& dir, list<string>& files, bool includeDirs, bool includeFiles, bool recursive, const string& filter), (const));
    MOCK_METHOD(string, getFileNameFromPath, (const string& path), (const));
};

// 全局mock实例
MockFileLibrary* g_mockFileLibrary = nullptr;

// 重写FileLibrary的getInstance方法以返回mock对象
namespace {
    FileLibrary* FileLibrary::getInstance() {
        return reinterpret_cast<FileLibrary*>(g_mockFileLibrary);
    }
}

// 测试夹具类
class ParserTest : public Test {
protected:
    void SetUp() override {
        g_mockFileLibrary = new MockFileLibrary();
    }
    
    void TearDown() override {
        delete g_mockFileLibrary;
        g_mockFileLibrary = nullptr;
    }
    
    // 辅助函数：创建模拟命令行参数
    vector<char*> createMockArgs(const vector<string>& args) {
        vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        return argv;
    }
};

// 测试用例：参数解析器正常情况
TEST_F(ParserTest, ParserNormalCase) {
    // 准备测试数据
    vector<string> args = {"program", "-test", "123"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    string name = "-test";
    int value = 0;
    
    // 执行测试
    int result = parser(argc, argv.data(), name, value);
    
    // 验证结果
    EXPECT_EQ(result, 0); // 应该返回0（index-2）
    EXPECT_EQ(value, 123); // 应该正确解析值
}

// 测试用例：参数不存在的情况
TEST_F(ParserTest, ParserArgumentNotFound) {
    vector<string> args = {"program", "-other", "456"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    string name = "-test";
    int value = 0;
    
    int result = parser(argc, argv.data(), name, value);
    
    EXPECT_EQ(result, -2); // 应该返回-2（index-2，其中index=0）
    EXPECT_EQ(value, 0); // 值应该保持不变
}

// 测试用例：参数在最后位置
TEST_F(ParserTest, ParserArgumentAtEnd) {
    vector<string> args = {"program", "-test"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    string name = "-test";
    int value = 0;
    
    int result = parser(argc, argv.data(), name, value);
    
    EXPECT_EQ(result, -1); // 应该返回-1（index-2，其中index=1）
    EXPECT_EQ(value, 0); // 值应该保持不变（没有对应的值）
}

// 测试用例：不同类型的参数解析
TEST_F(ParserTest, ParserDifferentTypes) {
    vector<string> args = {"program", "-int", "42", "-float", "3.14", "-string", "hello"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    // 测试整数
    int intValue = 0;
    int intResult = parser(argc, argv.data(), "-int", intValue);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(intValue, 42);
    
    // 测试浮点数
    float floatValue = 0.0f;
    int floatResult = parser(argc, argv.data(), "-float", floatValue);
    EXPECT_EQ(floatResult, 0);
    EXPECT_FLOAT_EQ(floatValue, 3.14f);
    
    // 测试字符串
    string stringValue = "";
    int stringResult = parser(argc, argv.data(), "-string", stringValue);
    EXPECT_EQ(stringResult, 0);
    EXPECT_EQ(stringValue, "hello");
}

// 测试用例：边界情况 - 空参数列表
TEST_F(ParserTest, ParserEmptyArguments) {
    vector<string> args = {"program"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    string name = "-test";
    int value = 999;
    
    int result = parser(argc, argv.data(), name, value);
    
    EXPECT_EQ(result, -2);
    EXPECT_EQ(value, 999); // 值应该保持不变
}

// 测试用例：文件存在性检查
TEST_F(ParserTest, FileExistenceCheck) {
    // 设置mock期望
    EXPECT_CALL(*g_mockFileLibrary, isFileExists(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    
    string existingFile = "/path/to/existing/file.obj";
    string nonExistingFile = "/path/to/nonexisting/file.obj";
    
    // 测试文件存在
    bool exists1 = FileLibrary::getInstance()->isFileExists(existingFile);
    EXPECT_TRUE(exists1);
    
    // 测试文件不存在
    bool exists2 = FileLibrary::getInstance()->isFileExists(nonExistingFile);
    EXPECT_FALSE(exists2);
}

// 测试用例：错误处理 - 无效的参数格式
TEST_F(ParserTest, ParserInvalidFormat) {
    vector<string> args = {"program", "-number", "not_a_number"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    int value = 100;
    int result = parser(argc, argv.data(), "-number", value);
    
    // 即使格式无效，也应该返回正确的位置
    EXPECT_EQ(result, 0);
    // 值可能保持不变或设置为0，取决于istringstream的行为
}

// 测试用例：多个相同参数的情况
TEST_F(ParserTest, ParserMultipleSameArguments) {
    vector<string> args = {"program", "-param", "first", "-param", "second"};
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    string name = "-param";
    string value = "";
    
    int result = parser(argc, argv.data(), name, value);
    
    // 应该找到第一个匹配的参数
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, "first");
}

// 性能测试：大量参数解析
TEST_F(ParserTest, ParserPerformance) {
    vector<string> args = {"program"};
    // 添加大量参数
    for (int i = 0; i < 100; i++) {
        args.push_back("-param" + to_string(i));
        args.push_back(to_string(i));
    }
    
    auto argv = createMockArgs(args);
    int argc = args.size();
    
    // 测试最后一个参数
    string name = "-param99";
    int value = 0;
    
    auto start = chrono::high_resolution_clock::now();
    int result = parser(argc, argv.data(), name, value);
    auto end = chrono::high_resolution_clock::now();
    
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    EXPECT_EQ(result, 196); // 应该返回正确的位置
    EXPECT_EQ(value, 99);
    EXPECT_LT(duration.count(), 1000); // 应该在1毫秒内完成
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}