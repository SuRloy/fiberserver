#ifndef __ZY_THREAD_H__
#define __ZY_THREAD_H__

#include <string>
#include <memory>
#include <functional>
#include "utils/mutex.h"
#include "utils/noncopyable.h"

namespace zy {


class Thread : NonCopyable {
public:
    using ptr = std::shared_ptr<Thread>;
    using thread_func = std::function<void()>;

    /**
     * @brief 构造函数
     * @param name 线程名
     * @param func 线程内执行的函数
     */
    Thread(const std::string name, thread_func cb);

    /**
     * @brief 析构函数
     * @details 使用 detach 系统调用使得主线程与子线程分离，子线程结束后，资源自动回收
     * @note 如果用户在构造函数之后调用了 join，那么这里的 detach 就没有作用了，因为 join 之后子线程已经结束了。
     */
    ~Thread();

    /**
     * @brief 阻塞等待线程执行完成
     * @details 构造函数返回时，线程入口函数已经在执行了，join 方法是为了让主线程等待子线程执行完成。
     */
    void join();

    /**
     * @brief 获取当前正在执行的线程指针
     * @return 当前正在执行的线程
     */
    static Thread* GetThis();

    uint32_t getId() const { return id_;}
    const std::string& getName() const { return name_;}

private:
    /**
     * @brief 线程入口函数
     * @param arg 线程执行时携带的参数
     * @return 线程执行返回的结果
     */
    static void *MainThread(void *arg);

private:
    /// 线程结构，标识一个线程
    pthread_t thread_{};
    /// 线程内部需要执行的函数
    thread_func cb_;
    /// 线程名
    std::string name_;
    /// 线程 id
    uint32_t id_;
    /// 信号量，用来保证线程初始化完成
    Semaphore sem_;
};

}

#endif