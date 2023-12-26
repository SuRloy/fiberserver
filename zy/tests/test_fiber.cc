#include "zy/zy.h"

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

void run_in_fiber() {
    ZY_LOG_INFO(g_logger) << "run_in_fiber begin";
    zy::Fiber::YieldToHold();
    ZY_LOG_INFO(g_logger) << "run_in_fiber end";
    zy::Fiber::YieldToHold();
}

void test_fiber() {
    ZY_LOG_INFO(g_logger) << "main begin -1";
    {
        zy::Fiber::GetThis();
        ZY_LOG_INFO(g_logger) << "main begin";
        zy::Fiber::ptr fiber(new zy::Fiber(run_in_fiber));
        fiber->swapIn();
        ZY_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        ZY_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    ZY_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    zy::Thread::SetName("main");

    std::vector<zy::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i) {
        thrs.push_back(zy::Thread::ptr(
                    new zy::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for (auto i : thrs) {
        i->join();
    }
    return 0;
}