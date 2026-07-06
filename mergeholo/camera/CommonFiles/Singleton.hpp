#ifndef SINGLETON_HPP
#define SINGLETON_HPP
#include <memory>
#include <mutex>

template <typename T>
class Singleton {
public:
    // 设置初始化参数
    template <typename... Args>
    static void initialize(Args&&... args) {

        Singleton& obj = getInstance();
        obj.m_pEntity = std::make_unique<T>(std::forward<Args>(args)...);
    }

    static T&  get()
    {
        auto& obj = getInstance();
        return *obj.m_pEntity;
    }

    // 获取单例实例
    static Singleton& getInstance() {
        // 确保只初始化一次
        static std::once_flag initFlag; // 用于确保线程安全的单例初始化
        static std::unique_ptr<Singleton<T> > instance;
        // 确保只初始化一次
        std::call_once(initFlag, [&]() {
            instance = std::make_unique<Singleton<T>>();
        });


        return *instance;
    }

    // 删除拷贝构造函数和赋值运算符，防止复制
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    Singleton() {} // 保护构造函数，以防外部实例化

private:
     // 存储单例实例
    std::unique_ptr<T> m_pEntity;
};
#endif
