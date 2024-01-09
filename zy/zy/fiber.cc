#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace zy {

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

// 当前协程，初始化为主协程
static thread_local Fiber* t_fiber = nullptr;
// 主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

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

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

//主协程的构造
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    // 获取当前协程的上下文信息保存到m_ctx中
    if (getcontext(&m_ctx)) {
        ZY_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    ZY_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

//子协程的构造
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    // 若给了初始化值则用给定值，若没有则用约定值
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    // 获得协程运行指针
    m_stack = StackAllocator::Alloc(m_stacksize);
    // 保存当前协程上下文信息到m_ctx中
    if (getcontext(&m_ctx)) {
        ZY_ASSERT2(false, "getcontext");
    }
    // uc_link为空，执行完当前context之后退出程序。
    m_ctx.uc_link = nullptr;
    // 初始化栈指针
    m_ctx.uc_stack.ss_sp = m_stack;
    // 初始化栈大小
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::Mainfunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainfunc, 0);
    }

    ZY_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    // 子协程
    if (m_stack) {
        // 不在准备和运行状态
        ZY_ASSERT(m_state == TERM 
                || m_state == EXCEPT
                || m_state == INIT);
        // 释放运行栈
        MallocStackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        ZY_ASSERT(!m_cb);
        ZY_ASSERT(m_state == EXEC);
        //主协程的释放要保证没有任务并且当前正在运行
        //若当前协程为主协程，将当前协程置为空
        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    ZY_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}
//重置协程函数，并重置状态INIT,TERM
void Fiber::reset(std::function<void()> cb) {
    ZY_ASSERT(m_stack);
    ZY_ASSERT(m_state == TERM 
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        ZY_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::Mainfunc, 0);
    m_state = INIT;
}

// 从调度器的主协程切换到当前协程
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    //ZY_LOG_ERROR(g_logger) << getId();
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        ZY_ASSERT2(false, "swapcontext");
    }
}

// 从当前协程切换到调度器主协程
void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        ZY_ASSERT2(false, "swapcontext");
    }
}

// 从协程主协程切换到当前协程
void Fiber::swapIn() {
    SetThis(this);
    ZY_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        ZY_ASSERT2(false, "swapcontext");
    }
}

// 从当前协程切换到主协程
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        ZY_ASSERT2(false, "swapcontext");
    }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}
//返回当前协程,没有协程说明目前是主协程
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    // 获得主协程
    Fiber::ptr main_fiber(new Fiber);
    ZY_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}
//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}
//协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}
//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::Mainfunc() {
    // 获得当前协程
    Fiber::ptr cur = GetThis();
    ZY_ASSERT(cur);
    try {
        // 执行任务
        cur->m_cb();
        cur->m_cb = nullptr;
        // 将状态设置为结束
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        ZY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        ZY_LOG_ERROR(g_logger) << "Fiber Except: "
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::BacktraceToString();
    }
    // 获得裸指针
    auto raw_ptr = cur.get();
    // 引用-1，防止fiber释放不掉
    cur.reset();
    //执行完释放执行权
    raw_ptr->swapOut();

    ZY_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

//use_caller运行的方法
void Fiber::CallerMainfunc() {
    Fiber::ptr cur = GetThis();
    ZY_ASSERT(cur);
    try {
        // 执行任务
        cur->m_cb();
        cur->m_cb = nullptr;
        // 将状态设置为结束
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        ZY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        ZY_LOG_ERROR(g_logger) << "Fiber Except: "
            << " fiber_id=" << cur->getId()
            << std::endl
            << zy::BacktraceToString();
    }
    
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

    ZY_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}


}