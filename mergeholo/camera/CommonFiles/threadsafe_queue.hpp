#pragma once // 防止头文件被重复包含
#ifndef THREAD_SAFE_QUEUE
#define THREAD_SAFE_QUEUE
#include <mutex>              // 包含互斥锁库
#include <queue>              // 包含队列库
#include <iostream>           // 包含输入输出流
#include <condition_variable> // 包含条件变量库
using namespace std;          // 使用标准命名空间

namespace JP
{
    /**
     * @brief 线程安全的队列模板类
     *
     * @tparam T 队列中元素的类型，默认为int
     */
    template <typename T = int>
    class threadsafe_queue
    {
        using milli_t = std::chrono::milliseconds; // 定义毫秒类型
    public:
        /**
         * @brief 构造函数
         *
         * @param iSize 队列的初始大小，默认为50
         * @param strName 队列的名称，默认为"queue"
         */
        threadsafe_queue(int iSize = 50, std::string strName = "queue")
            : qSize(iSize), m_strName(strName)
        {
        }

        /**
         * @brief 向队列中推入新元素，左值版本
         *
         * @param new_value 待推入的元素
         */
        void push(T &new_value)
        {
            std::lock_guard<std::mutex> lk(mut); // 互斥锁，保证线程安全
            if (size_flag && data_queue.size() >= static_cast<size_t>(qSize))
            {
                // 如果队列大小达到上限，删除队首元素
                data_queue.pop();
            }

            data_queue.push(new_value); // 推入新元素
            data_cond.notify_one();     // 唤醒一个等待线程
        }

        /**
         * @brief 向队列中推入新元素，右值版本
         *
         * @param new_value 待推入的元素
         */
        void push(T &&new_value)
        {
            std::lock_guard<std::mutex> lk(mut); // 互斥锁，保证线程安全
            if (size_flag && data_queue.size() >= static_cast<size_t>(qSize))
            {
                // 如果队列大小达到上限，删除队首元素
                data_queue.pop();
            }

            data_queue.push(new_value); // 推入新元素
            data_cond.notify_one();     // 唤醒一个等待线程
        }

        /**
         * @brief 从队列中弹出元素
         *
         * @param value 保存弹出元素的引用
         * @param timeout 超时时间，默认为1毫秒
         * @return true 如果成功弹出元素
         * @return false 如果超时或队列为空
         */
        bool pop(T &value, int timeout = 1)
        {
            std::unique_lock<std::mutex> lk(mut); // 独占锁，保证线程安全
            milli_t ms(timeout);                  // 转换为毫秒
            bool flag = data_cond.wait_for(lk, ms, [this]
                                           { return !data_queue.empty(); }); // 等待条件
            if (!flag)
                return false; // 如果条件不满足，返回false

            value = data_queue.front(); // 获取队首元素
            data_queue.pop();           // 弹出队首元素

            // 确保队列大小不超过上限
            while (size_flag && data_queue.size() > static_cast<size_t>(qSize))
            {
                value = data_queue.front();
                data_queue.pop();
            }

            return true;
        }

        /**
         * @brief 检查队列是否为空
         *
         * @return true 队列为空
         * @return false 队列不为空
         */
        bool empty() const
        {
            // std::lock_guard<std::mutex> lk(mut);  // 注释掉，防止死锁，但保持队列的状态一致
            return data_queue.empty(); // 返回队列是否为空
        }

        /**
         * @brief 获取队列的大小
         *
         * @return int 队列的大小
         */
        int size() const
        {
            std::lock_guard<std::mutex> lk(mut); // 互斥锁，保证线程安全
            return (int)data_queue.size();       // 返回队列的大小
        }

        /**
         * @brief 清空队列
         */
        void clear()
        {
            std::lock_guard<std::mutex> lk(mut); // 互斥锁，保证线程安全
            while (!data_queue.empty())
            {
                data_queue.pop(); // 弹出所有元素
            }
        }

        /**
         * @brief 关闭队列大小限制
         */
        void setflag()
        {
            size_flag = false; // 将大小限制标志设为false
        }

        /**
         * @brief 设置队列的容量
         *
         * @param iMaxQueueSize 最大队列容量
         */
        void setCapacity(int iMaxQueueSize)
        {
            qSize = iMaxQueueSize; // 设置队列的最大容量
        }

    private:
        mutable std::mutex mut;            // 互斥量，保证线程安全
        std::queue<T> data_queue;          // 队列容器
        std::condition_variable data_cond; // 条件变量，用于线程间的通知

        int qSize = 1;                   // 队列的初始大小
        bool size_flag = true;           // 大小限制标志
        std::string m_strName = "queue"; // 队列的名字
    };
};
#endif