#include "zy/zy.h"

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

void run_in_fiber() {
    ZY_LOG_INFO(g_logger) << "run_in_fiber begin";
    zy::Fiber::YieldToHold();
    ZY_LOG_INFO(g_logger) << "run_in_fiber end";
    //zy::Fiber::YieldToHold();
}

int main(int argc, char** argv) {
    zy::Fiber::GetThis();
    ZY_LOG_INFO(g_logger) << "main begin";
    zy::Fiber::ptr fiber(new zy::Fiber(run_in_fiber));
    fiber->swapIn();
    ZY_LOG_INFO(g_logger) << "main after swapIn";
    fiber->swapIn();
    ZY_LOG_INFO(g_logger) << "main after end";
    return 0;
}