#include "scheduler.h"
#include "log.h"
#include "utils/macro.h"

namespace zy {

// 当前线程所属的调度器，同一个调度器下的所有线程共享同一个实例
static thread_local Scheduler* t_scheduler = nullptr;
// 当前线程的调度协程，调度器所在线程的调度协程不是主协程，其余线程的调度协程为主协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(std::string name, uint32_t thread_num, bool use_caller)
    : name_(std::move(name)), stopping_(false), thread_num_(thread_num)
    , active_thread_num_(0), idle_thread_num_(0)
    , use_caller_(use_caller), caller_tid_(-1) {
    ZY_ASSERT(thread_num > 0);
    setThreadName(name_);

    // 初始化主线程的主协程，即调度器所在的协程
    Fiber::InitMainFiber();    
    if (use_caller) {
        ZY_ASSERT(GetThis() == nullptr);

        // 再初始化一个主线程的调度协程，使用 Scheduler::run 作为入口函数
        // 调度器所在线程的主协程和调度协程不是同一个
        // 该调度协程会在调度器退出前执行
        caller_fiber_.reset(new Fiber(std::bind(&Scheduler::run, this), false));
        caller_tid_ = getThreadId();

        // 调度器所在线程，即主线程的线程局部变量初始化
        SetThis(this);
        t_scheduler_fiber = caller_fiber_.get();
    }
}

Scheduler::~Scheduler() {
    ZY_ASSERT(stopping_ == true);
    if (GetThis() == this) {
        // 析构函数会在主线程的主协程内被调用
        // GetThis() != this 的情况只有 GetThis() == nullptr，即没有给它赋值
        // 可能出现的时机为 use_caller_ = false
        t_scheduler = nullptr;
    }
}

//核心 线程池
void Scheduler::start() {
    Mutex::Lock lock(mutex_);
    if (!stopping_) {
        return;
    }
    stopping_ = false;
    ZY_ASSERT(threads_.empty());

    threads_.resize(thread_num_);
    // 创建对应数量的子线程，子线程的入口函数也是子线程主协程的入口函数
    for (uint32_t i = 0; i < thread_num_; ++i) {
        threads_[i].reset(
                new Thread(name_ + "_" + std::to_string(i), std::bind(&Scheduler::run, this))
        );
    }
}

void Scheduler::stop() {
    stopping_ = true;

    // use_caller线程
    // 当前调度器和t_secheduler相同
    if (use_caller_) {
        ZY_ASSERT(GetThis() == this);
    } else {// 非use_caller，此时的t_secheduler应该为nullptr
        ZY_ASSERT(GetThis() != this);
    }

    // 通知所有子线程的调度协程退出调度, thread_num_计数除开调度器所在线程，所以下方要多tickle一次
    for (size_t i = 0; i < thread_num_; ++i) {
        tickle();
    }
    // 通知主线程的调度协程退出调度
    if (caller_fiber_) {
        tickle();
    }

    // 使用use_caller，只要没达到停止条件，调度器退出时要调用 caller_fiber_
    if (caller_fiber_) {
        caller_fiber_->resume();
    }

    std::vector<Thread::ptr> thrs;
    {
        Mutex::Lock lock(mutex_);
        thrs.swap(threads_);
    }

    // 等待所有任务都调度完成后才可以退出
    for (auto& i : thrs) {
        i->join();
    }
    // if (exit_on_this_fiber)
}

bool Scheduler::stopping() {
    Mutex::Lock lock(mutex_);
    // 所有任务都执行结束才可以停止调度器
    return stopping_ && tasks_.empty() && active_thread_num_ == 0;
}

void Scheduler::run() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "run begin";
    //setHooked(true);
    SetThis(this);

    // 非user_caller线程，设置主协程为线程主协程
    if (getThreadId() != caller_tid_) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    // 新建dile_fiber，当任务队列中的任务执行完之后，执行idle()
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    static thread_local SchedulerTask task;
    while (true) {
        task.reset();
        bool tickle_me = false;
        {	// 从任务队列中拿fiber和cb
            Mutex::Lock lock(mutex_);
            auto it = tasks_.begin();
            while (it != tasks_.end()) {
                //it的协程并非指名的协程，则跳过，并且tickle一下
                if (it->tid_ != INVALID_TID && it->tid_ != zy::getThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // [BUG FIX]: hook IO 相关的系统调用时，在检测到 IO 未就绪的情况下，会先添加对应的读写事件，再 yield 当前协程，
                // 等 IO 就绪后再 resume 当前协程。多线程高并发情境下，有可能发生刚添加事件就被触发的情况，如果此时当前协程还未来得及
                // yield，则这里就有可能出现协程状态仍为 RUNNING 的情况。这里简单地跳过这种情况，以损失一点性能为代价，
                // 指名的协程正在工作
                if (it->fiber_ && it->fiber_->getState() == Fiber::RUNNING) {
                    ++it;
                    continue;
                }

                ZY_ASSERT(it->fiber_ || it->cb_);
                //非上述两种情况才处理
                task = *it;
                tasks_.erase(it);
                ++active_thread_num_;
                break;
            }
            // 当前调度协程拿走一个任务后，还有剩余任务，也需要通知其他线程继续调度
            tickle_me |= (it != tasks_.end());
        }

        if (tickle_me) {
            tickle();
        }

        // 如果任务是fiber，并且任务处于可执行状态
        if (task.fiber_) {                                          // 协程直接调度
            task.fiber_->resume();
            --active_thread_num_;
        } else if (task.cb_) {
            Fiber::ptr func_fiber(new Fiber(task.cb_));   // 函数封装成协程再调度
            func_fiber->resume();
            --active_thread_num_;
        } else {                                                    // 没有任务了，进入到 idle 协程
            if (idle_fiber->getState() == Fiber::TERM) {            // idle 协程在满足退出条件后会退出，执行状态变成 TERM
                break;
            }
            ++idle_thread_num_;
            idle_fiber->resume();
            --idle_thread_num_;
        }
    }
}

void Scheduler::tickle() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "tickle";
}



void Scheduler::idle() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "idle";
    // 此处的 idle 协程什么都不做，进来就退出
    // 调度器可以停止时，就会退出 while 循环，idle 协程就执行结束了，协程状态变味了 TERM
    // 之后 Scheduler::run() 的 while(true) 就会结束
    while (!stopping()) {
        Fiber::GetThis()->yield();
    }
}

Scheduler *Scheduler::GetThis() {
    return t_scheduler;
}

void Scheduler::SetThis(Scheduler *scheduler) {
    t_scheduler = scheduler;
}

Fiber *Scheduler::GetSchedulerFiber() {
    return t_scheduler_fiber;
}


}