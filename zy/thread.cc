#include "thread.h"
#include "log.h"
#include "utils/util.h"

namespace zy {
//static thread_local 通常用于修饰静态变量，以便每个线程都有一个独立的静态变量实例
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "";


Thread::Thread(const std::string name, std::function<void()> cb)
    :cb_(std::move(cb)), name_(std::move(name)), id_(-1) {
    if(name.empty()) {
        name_ = "UNKOWN";
    }
    // pthread_create 执行之后，Thread::MainThread 就已经开始运行
    int rt = pthread_create(&thread_, nullptr, &Thread::MainThread, this);
    if(rt) {
        ZY_LOG_ERROR(ZY_LOG_ROOT()) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    sem_.wait();//保证回来之后线程Id初始好
}

Thread::~Thread() {
        // // 如果主线程结束时子线程还没结束，那么分离主线程和子线程
        if (thread_) {
            int rt = pthread_detach(thread_);
            if (rt) {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "pthread_detach error";
            }
        }
}

//join 的作用是主线程等待被调用 join 的线程执行完毕。pthread_join需要阻塞等待线程结束并获取其[退出状态]
//这是一种协同的线程管理方式，确保主线程在子线程结束之前不会继续执行
void Thread::join() {
    if(thread_) {
        int rt = pthread_join(thread_, nullptr);//如果有值说明线程还未结束
        if(rt) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "pthread_join thread fail, rt=" << rt
                << " name=" << name_;
            throw std::logic_error("pthread_join error");
        }
        thread_ = 0;
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}


void* Thread::MainThread(void* arg) {
    Thread* thread = static_cast<Thread *>(arg);
    // 设置当前线程的 id
    thread->id_ = getThreadId();
    // 设置当前线程的名字
    setThreadName(thread->getName());
    
    t_thread = thread;
    // swap 交换空间之后，两个线程的执行就不会相互影响了
    std::function<void()> cb;
    cb.swap(t_thread->cb_);

    t_thread->sem_.notify();
    cb();
    return 0;
}


}