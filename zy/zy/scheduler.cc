#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace zy {

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    ZY_ASSERT(threads > 0);

    if (use_caller) {
        zy::Fiber::GetThis();
        --threads;

        ZY_ASSERT(Getthis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        zy::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThread = zy::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    ZY_ASSERT(m_stopping);
    if (Getthis() == this) {
        t_scheduler = nullptr;
    }
}


Scheduler* Scheduler::Getthis() {
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
    if (m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM 
                || m_rootFiber->getState() == Fiber::INIT)) {
        ZY_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    //bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {
        ZY_ASSERT(Getthis() == this);
    } else {
        ZY_ASSERT(Getthis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        // while(!stopping()) {
        //     if (m_rootFiber->getState() == Fiber::TERM
        //             || m_rootFiber->getState() == Fiber::EXCEPT) {
        //                 m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //                 ZY_LOG_INFO(g_logger) << " root fiber is term, reset";
        //                 t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
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

    if (zy::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();
    }
    
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                //it的协程并非指名的协程
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

        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
                            && ft.fiber->getState() != Fiber::EXCEPT)) {
            
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::TERM
                    || cb_fiber->getState() == Fiber::EXCEPT) {
                cb_fiber->reset(nullptr);
            } else {//(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                ZY_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
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