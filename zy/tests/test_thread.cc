#include "../zy/zy.h"
#include <unistd.h>

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

void fun1() {
    ZY_LOG_INFO(g_logger) << "name: " << zy::Thread::GetName()
                             << " this.name: " << zy::Thread::GetThis()->getName()
                             << " id: " << zy::GetThreadId()
                             << " this.id: " << zy::Thread::GetThis()->getId();    
    sleep(30);
}

void fun2() {

}

int main(int argc, char** argv) {
    ZY_LOG_INFO(g_logger) << "thread test begin";
    std::vector<zy::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        zy::Thread::ptr thr(new zy::Thread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    
    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    ZY_LOG_INFO(g_logger) << "thread test end";
    return 0;
}