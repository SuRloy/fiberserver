#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace zy {

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

// 当前协程调度器
static thread_local Scheduler* t_scheduler = nullptr;
// 线程主协程
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    ZY_ASSERT(threads > 0);

    if (use_caller) {
        zy::Thread::SetName(m_name);
        zy::Fiber::GetThis();
        --threads;

        ZY_ASSERT(GetThis() == nullptr);
        // 设置当前协程调度器
        t_scheduler = this;

        // 将此调度协程fiber设置为 use_caller，协程则会与 Fiber::CallerMainFunc() 绑定
        // 非静态成员函数需要传递this指针作为第一个参数，用 std::bind()进行绑定
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));

        // 设置当前线程的主协程为m_rootFiber
        t_fiber = m_rootFiber.get();

        //主线程Id
        m_rootThread = zy::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    ZY_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}


Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}

//核心 线程池
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    ZY_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        // 线程执行 run() 任务
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if (m_rootFiber) {
    //     //m_rootFiber->swapIn();
    //     m_rootFiber->call();
    //     ZY_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop() {
    m_autoStop = true;
    // 使用use_caller,并且只有一个线程，并且主协程的状态为结束或者初始化
    if (m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM 
                || m_rootFiber->getState() == Fiber::INIT)) {
        ZY_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;
        // 若达到停止条件则直接return
        if (stopping()) {
            return;
        }
    }

    // use_caller线程
    // 当前调度器和t_secheduler相同
    if (m_rootThread != -1) {
        ZY_ASSERT(GetThis() == this);
    } else {// 非use_caller，此时的t_secheduler应该为nullptr
        ZY_ASSERT(GetThis() != this);
    }

    // 每个线程都tickle一下
    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    // 使用use_caller多tickle一下
    if (m_rootFiber) {
        tickle();
    }

    // 使用use_caller，只要没达到停止条件，调度器主协程交出执行权，执行run
    if (m_rootFiber) {
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto& i : thrs) {
        i->join();
    }
    // if (exit_on_this_fiber)
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    ZY_LOG_INFO(g_logger) << "run";
    setThis();

    // 非user_caller线程，设置主协程为线程主协程
    if (zy::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();
    }
    // 定义dile_fiber，当任务队列中的任务执行完之后，执行idle()
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {	// 从任务队列中拿fiber和cb
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                //it的协程并非指名的协程，则跳过，并且tickle一下
                if (it->thread != -1 && it->thread != zy::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                ZY_ASSERT(it->fiber || it->cb);
                //指名的协程正在工作
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                //非上述两种情况才处理
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            //tickle_me |= it != m_fibers.end();
        }

        if (tickle_me) {
            tickle();
        }

        // 如果任务是fiber，并且任务处于可执行状态
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
                            && ft.fiber->getState() != Fiber::EXCEPT)) {
            // 执行任务
            ft.fiber->swapIn();
            // 执行完成，活跃的线程数量减-1
            --m_activeThreadCount;
            // 如果线程的状态被置为了READY
            if (ft.fiber->getState() == Fiber::READY) {
                // 将fiber重新加入到任务队列中
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM // INIT或HOLD状态
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {// 如果任务是cb
            // cb_fiber存在，重置该fiber
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {// cb_fiber不存在，new新的fiber
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            // 执行cb任务
            cb_fiber->swapIn();
            // 执行完成，活跃的线程数量减-1
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                // 重新放入任务队列中
                schedule(cb_fiber);
                // 释放智能指针
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::TERM
                    || cb_fiber->getState() == Fiber::EXCEPT) {// cb_fiber异常或结束，就重置状态，可以再次使用该cb_fiber
                // cb_fiber的执行任务置空
                cb_fiber->reset(nullptr);
            } else {
                // 设置状态为HOLD，此任务后面还会通过ft.fiber被拉起
                cb_fiber->m_state = Fiber::HOLD;
                // 释放该智能指针，调用下一个任务时要重新new一个新的cb_fiber
                cb_fiber.reset();
            }
        } else {// 没有任务执行
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            // 如果idle_fiber的状态为TERM则结束循环，真正的结束
            if(idle_fiber->getState() == Fiber::TERM) {
                ZY_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            // 正在执行idle的线程数量+1
            ++m_idleThreadCount;
            // 执行idle()
            idle_fiber->swapIn();
            // 正在执行idle的线程数量-1
            --m_idleThreadCount;
            // idle_fiber状态置为HOLD
            if (idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->m_state = Fiber::HOLD;
            }
            
        }
    }
}

void Scheduler::tickle() {
    ZY_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    // 当自动停止 || 正在停止 || 任务队列为空 || 活跃的线程数量为0
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    ZY_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        zy::Fiber::YieldToHold();
    }
}



}