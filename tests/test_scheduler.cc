//
// Created by liucxi on 2022/11/8.
//

#include "scheduler.h"
#include "utils/util.h"
#include <iostream>
#include "log.h"
#include <unistd.h>

using namespace zy;

void test_scheduler1() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler1 begin";

    Scheduler::GetThis()->addTask(Fiber::GetThis()->shared_from_this());            // yield()前必须将自己再加入协程调度器
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "before test_scheduler1 yield";
    Fiber::GetThis()->yield();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after test_scheduler1 yield";

    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler1 end";
}

void test_scheduler2() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler2 begin";

    sleep(3);

    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler2 end";
}

void test_scheduler3() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler3 begin";

    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler3 end";
}

void test_scheduler4() {
    static int count = 0;
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler4 begin, i = " << count;
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler4 end, i = " << count;
    ++count;
}

void test_scheduler5() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler5 begin";
    for (int i = 0; i < 3; ++i) {
        Scheduler::GetThis()->addTask(test_scheduler4, getThreadId());
    }
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " test_scheduler5 end";
}

int main() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " main begin";
    //Scheduler scheduler("scheduler");
    //Scheduler scheduler("scheduler", 1, false);
    Scheduler scheduler("scheduler", 1, true);

    // scheduler.addTask(test_scheduler1);
    // scheduler.addTask(test_scheduler2);

    scheduler.addTask(std::make_shared<Fiber>(test_scheduler3));//用户自己创建fiber

    scheduler.start();//创建指定数量线程并绑定scheduler::run函数

    scheduler.addTask(test_scheduler5);//test_scheduler5创建了三个任务，加入任务队列后创建任务协程调度完成任务并结束


    scheduler.stop();//stop触发

    ZY_LOG_INFO(ZY_LOG_ROOT()) << getThreadId() << ", " << Fiber::GetFiberId() << " main end";
    return 0;
}
