#include "thread.h"
#include "log.h"

namespace zy {
//static thread_local 通常用于修饰静态变量，以便每个线程都有一个独立的静态变量实例
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);//如果有值说明已经创建了
    if(rt) {
        ZY_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    //m_semaphore.wait();
}

Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

//join 的作用是主线程等待被调用 join 的线程执行完毕。
//这是一种协同的线程管理方式，确保主线程在子线程结束之前不会继续执行
void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);//如果有值说明线程还未结束
        if(rt) {
            ZY_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = zy::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);//TODO:有特殊用处

    //thread->m_semaphore.notify();

    cb();
    return 0;
}


}