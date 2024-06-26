#ifndef __ZY_SCHEDULER_H__
#define __ZY_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"

namespace zy {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name;}

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            // 将任务加入到队列中，若任务队列中已经有任务了，则tickle()
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    //批量存入
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle =  scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
            if (need_tickle) {
                tickle();
            }
        }
    }
protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle();

    void setThis();

    bool hasIdleThread() { return m_idleThreadCount > 0;}
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);//存入fiber列表中
        }
        return need_tickle;
    }
private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        // 线程id 协程在哪个线程上 
        int thread;

        // 确定协程在哪个线程上跑
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {}
        
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
                fiber.swap(*f);//智能指针f变成空指针，指针计数器减一，防止出问题 智能指针本质上是使用局部变量生命周期管理堆内存
        }

         // 确定回调函数在哪个线程上跑
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {

        }

        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
                cb.swap(*f);
        }

        //STL内需要一个默认构造函数，否则分配出来的对象无法初始化，如下方list
        FiberAndThread() :thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    // 线程池
    std::vector<Thread::ptr> m_threads;
    // 待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    // use_caller为true时有效, 调度协程
    Fiber::ptr m_rootFiber;
    // 协程调度器名称
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = false;
    // 主线程Id(use_caller)
    int m_rootThread = 0;
};





}

#endif