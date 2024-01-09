#include "zy/zy.h"

static zy::Logger::ptr g_logger = ZY_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    ZY_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;
    sleep(1);
    if (--s_count >= 0) {
        zy::Scheduler::GetThis()->schedule(&test_fiber, zy::GetThreadId());
    }
}

int main(int argc, char** argv) {
    ZY_LOG_INFO(g_logger) << "main";
    zy::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    ZY_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    ZY_LOG_INFO(g_logger) << "over";
    return 0;
}