

#include <thread>
#include <iostream>
#include "fiber.h"
#include "log.h"

using namespace zy;

void run_in_fiber1() {
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "run in fiber1 start";
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "run in fiber1 yield";
    Fiber::GetThis()->yield();
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "run in fiber1 end";
}

void run_in_fiber2() {
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "run in fiber2 start";
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "run in fiber2 end";
}

// 线程函数
void test_fiber() {
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "test_fiber start";
    Fiber::InitMainFiber();
    Fiber::ptr fiber(new Fiber(run_in_fiber1, false));

    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "do something";
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "test_fiber yield";
    fiber->resume();
    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "test_fiber resume and yield";

    fiber->resume();

    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "test_fiber switch to run in fiber2";
    fiber->reset(run_in_fiber2);
    fiber->resume();

    ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "test_fiber end";
}
int main() {
    std::thread t(test_fiber);
    t.join();
    return 0;
}