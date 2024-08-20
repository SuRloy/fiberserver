#include "fiber.h"
#include "utils/macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>
#include <utility>

namespace zy {

// 只增不减，用来标识唯一的协程
static std::atomic<uint64_t> s_fiber_id {0};
// 可增可减，用来记录当前正在运行的线程数量（进程级别）
static std::atomic<uint64_t> s_fiber_num {0};

/**
 * @brief 当前线程正在执行的协程，裸指针
 * @note 为什么不使用智能指针？是因为有可能出现循环引用的问题。
 * 但更现实的问题是在实现过程中，无论是在 SetThis() 的参数中使用智能指针还是裸指针都会有问题。
 * 使用智能指针，在函数内赋值没问题，但是由于 SetThis() 会在构造函数内被调用，此时对象尚未构造完成，shared_from_this() 出错
 * 使用裸指针，在未构造完成的对象内使用 this 指针没问题，可以正常将值传递给 SetThis()，但是将裸指针赋值给智能指针会有问题。
 * 加上 t_thread_fiber 指示当前正在运行的协程，会不断改变指向，使用智能指针还有引用计数等的问题要分析，综上，使用裸指针实现简单。
 * 但是用户在构造 Fiber 对象时可能会使用 Fiber::ptr，所以还会出现裸指针与智能指针混用的问题，无论怎样都没有想到一个完美的解决方案。
 */
static thread_local Fiber* t_thread_fiber = nullptr;

// 主协程
static thread_local Fiber::ptr t_main_fiber = nullptr;
// 默认的协程栈空间大小
static const uint32_t stack_size = 128 * 1024;

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;


//主协程的构造
Fiber::Fiber()
    : id_(s_fiber_id++), state_(RUNNING), stack_size_(0), run_in_scheduler_(false) {
    ++s_fiber_num;
    SetThis(this);

    // 获取当前协程的上下文信息保存到ctx_中
    if (::getcontext(&ctx_)) {
        ZY_ASSERT2(false, "getcontext");
    }

    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "Fiber::Fiber main";
}

//子协程的构造
Fiber::Fiber(fiber_func func, bool run_in_scheduler)
        : id_(s_fiber_id++), state_(READY), cb_(std::move(func))
        , stack_size_(stack_size), run_in_scheduler_(run_in_scheduler) {
    ++s_fiber_num;

    // 获得协程运行指针
    stack_ = StackAllocator::Alloc(stack_size_);
    // 保存当前协程上下文信息到ctx_中
    if (::getcontext(&ctx_)) {
        ZY_ASSERT2(false, "getcontext");
    }

    // 当前上下文结束之后运行的上下文为空，那么在本协程运行结束时必须要调用 setcontext 或 swapcontext 以重新指定一个有效的上下文，
    // 否则程序就跑飞了，在代码中体现为 Fiber::MainFunc 的最后 yield 一次，在 yield 中恢复了主协程的运行

    // uc_link为空，执行完当前context之后退出程序。
    ctx_.uc_link = nullptr;
    // 初始化栈指针
    ctx_.uc_stack.ss_sp = stack_;
    // 初始化栈大小
    ctx_.uc_stack.ss_size = stack_size_;

    makecontext(&ctx_, &Fiber::Mainfunc, 0);

    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "Fiber::Fiber id = " << id_;
}

Fiber::~Fiber() {
    --s_fiber_num;
    // 子协程
    if (stack_) {
        // 不在准备和运行状态
        ZY_ASSERT(state_ == TERM);
        // 释放运行栈
        MallocStackAllocator::Dealloc(stack_, stack_size_);
    } else {
        ZY_ASSERT(!cb_);
        ZY_ASSERT(state_ == RUNNING);
        //主协程的释放要保证没有任务并且当前正在运行
        //若当前协程为主协程，将当前协程置为空
        Fiber* cur = t_thread_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "Fiber::~Fiber id = " << id_;
}

void Fiber::reset(std::function<void()> cb) {
    ZY_ASSERT(stack_);
    ZY_ASSERT(state_ == TERM);

    state_ = READY;
    cb_ = std::move(cb);
    if (getcontext(&ctx_)) {
        ZY_ASSERT2(false, "getcontext");
    }
    ctx_.uc_link = nullptr;
    ctx_.uc_stack.ss_sp = stack_;
    ctx_.uc_stack.ss_size = stack_size;

    makecontext(&ctx_, &Fiber::Mainfunc, 0);
}

void Fiber::yield() {
    ZY_ASSERT(state_ == RUNNING || state_ == TERM);

    if (state_ == RUNNING) {
        state_ = READY;
    }
    if (run_in_scheduler_) {
        SetThis(Scheduler::GetSchedulerFiber());        // 当前协程退出执行，要将线程正在执行的协程修改为调度协程
        if (swapcontext(&ctx_, &(Scheduler::GetSchedulerFiber()->ctx_))) {
            ZY_ASSERT2(false, "swapcontext error");
        }
    } else {
        SetThis(t_main_fiber.get());                    // 当前协程退出执行，要将线程正在执行的协程修改为主协程
        // 当前协程栈空间保存在第一个参数里，从第二个参数读出协程占空间恢复执行
        if (swapcontext(&ctx_, &(t_main_fiber->ctx_))) {
            ZY_ASSERT2(false, "swapcontext error");
        }
    }
}

void Fiber::resume() {
    ZY_ASSERT(state_ == READY);

    SetThis(this);                                  // 当前协程需要恢复执行，要将线程正在执行的协程修改为当前协程
    state_ = RUNNING;
    if (run_in_scheduler_) {
        if (swapcontext(&(Scheduler::GetSchedulerFiber()->ctx_), &ctx_)) {
            ZY_ASSERT2(false, "swapcontext error");
        }
    } else {
        // 当前协程栈空间保存在第一个参数里，从第二个参数读出协程占空间恢复执行
        if (swapcontext(&(t_main_fiber->ctx_), &ctx_)) {
            ZY_ASSERT2(false, "swapcontext error");
        }
    }
}

void Fiber::InitMainFiber() {
    t_main_fiber = Fiber::ptr(new Fiber);           // 创建主协程
    ZY_ASSERT(t_main_fiber);                         // 现在有主协程了
    ZY_ASSERT(t_thread_fiber == t_main_fiber.get()); // 当前正在运行的协程就是主协程
}

void Fiber::SetThis(Fiber* f) {
    t_thread_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    return t_thread_fiber->shared_from_this();
}

uint32_t Fiber::GetFiberId() {
    if (!t_thread_fiber) {
        return INVALID_TID;
    }
    return t_thread_fiber->getId();
}

uint32_t Fiber::TotalFibers() {
    return s_fiber_num;
}

void Fiber::Mainfunc() {
    // 获得当前协程
    auto cur = GetThis();
    ZY_ASSERT(cur);
    try {
        // 执行任务
        cur->cb_();
        cur->cb_ = nullptr;
        // 将状态设置为结束
        cur->state_ = TERM;
    } catch (std::exception& ex) {
        cur->state_ = EXCEPT;
        ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::backtraceToString();
    } catch (...) {
        cur->state_ = EXCEPT;
        ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Fiber Except: "
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::backtraceToString();
    }
    // 获得裸指针
    auto raw_ptr = cur.get();
    // 引用-1，防止fiber释放不掉
    cur.reset();
    //执行完释放执行权
    raw_ptr->yield();

    ZY_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}


}