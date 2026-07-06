/**
 * @file DataQueue.hpp
 * @author 罗志彬 (luozhibin@jumperscience.com)
 * @brief 用来管理整个程序需要的数据
 * @version 1.0.0.0
 * @date 2022-06-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef DATA_QUEUE_HPP
#define DATA_QUEUE_HPP

#include "CommonMacro.h"
#include "threadsafe_queue.hpp"

namespace JP
{

    /**
     * @brief 数据队列类
     *
     * @tparam QueueItemType 队列数据的类型
     */
    template <typename QueueItemType>
    class CDataQueue
    {
    public:
        /**
         * @brief 创建一个 CDataQueue 新对象
         *
         * @param size
         */
        CDataQueue(size_t size = 8)
            : mszSize(size), mqIdle(size)
        {
            resize(size);
        }

        /**
         * @brief 重置队列大小
         *
         * @param mszSize 新的队列大小
         */
        void resize(size_t szSize)
        {
            clear();

            mqBusy.setCapacity(szSize);
            mqIdle.setCapacity(szSize);
            std::lock_guard<std::mutex> lk(mut);
            while (mqIdle.size() > szSize)
            {
                QueueItemType tmp;
                mqIdle.pop(tmp);
                //printf("inti1 queu size %lu\n", mqIdle.size());
            }
            while (mqIdle.size() < szSize)
            {
                QueueItemType oNew;
                mqIdle.push(oNew);
                //printf("inti2 queu size %lu\n", mqIdle.size());
            }
            mszSize = szSize;

        }

        /**
         * @brief 获得队列大小
         *
         * @return size_t 当前队列的大小
         */
        size_t capacity() const
        {
            return mszSize;
        }


        size_t size() const
        {
            return mqBusy.size();
        }

        bool isFull()
        {
            return mszSize == mqBusy.size();
        }
        /**
         * @brief 从队列剩余空间中申请一个元素
         *
         * @param value 用来存储该元素的空间
         * @param timeout 超时
         * @return true 申请成功
         * @return false 申请失败
         */
        bool allocate(QueueItemType &value, int timeout = 1)
        {
            return mqIdle.pop(value, timeout);
        }

        /**
         * @brief 向队列中添加一个元素
         *
         * @param value 要添加的值
         */
        void pushQ(QueueItemType &value)
        {
            mqBusy.push(value);
        }

        /**
         * @brief 从队列中弹出第一个元素
         *
         * @param value 存储值的对象
         * @param timeout 超时时间，超过将失败， -1为永久
         * @return true 弹出成功
         * @return false 弹出失败
         */
        bool popQ(QueueItemType &value, int timeout = 1)
        {
            return mqBusy.pop(value, timeout);
        }

        /**
         * @brief 将申请的空间归还给队列，作为空闲内存使用
         *
         * @param value 要归还的空间
         */
        void release(QueueItemType &value)
        {
            mqIdle.push(value);
        }

        /**
         * @brief 清空队列，将队列所有元素置为闲置
         *
         */
        void clear()
        {
            std::lock_guard<std::mutex> lk(mut);
            while (!mqBusy.empty())
            {
                QueueItemType tmp;
                mqBusy.pop(tmp);
                mqIdle.push(tmp);
            }
        }

        /**
         * @brief 判断队列是否为空
         *
         * @return true 空队列
         * @return false 非空队列
         */
        bool empty()
        {
            std::lock_guard<std::mutex> lk(mut);
            return !mqBusy.empty();
        }

        /**
         * @brief 设置标志位，将队列设置为【超过容量则自动替换】
         *
         */
        void setFlag()
        {
            mqBusy.setflag();
            mqIdle.setflag();
        }

    public:
        /// @brief 占用的队列
        JP::threadsafe_queue<QueueItemType> mqBusy;
        /// @brief 空闲队列
        JP::threadsafe_queue<QueueItemType> mqIdle;

    private:
        /// @brief 写入锁
        mutable std::mutex mut;
        /// @brief 队列大小
        size_t mszSize;
    };    
}

#endif // DATA_QUEUE_HPP
