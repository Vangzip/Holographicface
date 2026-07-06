#ifndef IJPDEVINTERFACE_H
#define IJPDEVINTERFACE_H
#include <memory>
namespace JP
{
    class IDeviceInterface
    {
    public:

        using Ptr = std::shared_ptr<IDeviceInterface>;
    public:
        virtual ~IDeviceInterface() {}

        // 初始化设备
        virtual bool initialize( const void* config ) = 0;

        // 逆操作，释放资源
        virtual void release() = 0;

        // 打开设备
        virtual bool openDevice() = 0;

        // 关闭设备
        virtual void closeDevice() = 0;

        // 读取数据
        virtual int readData(char* buffer, int length) = 0;

        // 写入数据
        virtual bool writeData(const char* buffer, int length) = 0;
    };
}
#endif // IJPDEVINTERFACE_H
