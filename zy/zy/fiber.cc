#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>

namespace zy {

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 256 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAlloc = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        ZY_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = MallocStackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        ZY_ASSERT2(false, "setcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::Mainfunc, 0);
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        ZY_ASSERT(m_state == TERM || m_state == INIT);

        MallocStackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        ZY_ASSERT(!m_cb);
        ZY_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
}
void reset(std::function<void()> cb);//重置协程函数，并重置状态INIT,TERM
void swapIn();//切换到当前协程执行
void swapOut();//切换到后台执行

//设置当前协程
static void SetThis(Fiber* f);
//程序拿到自己的协程
static Fiber::ptr GetThis();
//协程切换到后台，并且设置为Ready状态
static void YieldToReady();
//协程切换到后台，并且设置为Hold状态
static void YieldToHold();
//总协程数
static uint64_t TotalFibers();
static void Mainfunc();
}