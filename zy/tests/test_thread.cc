#include "../zy/zy.h"
#include <unistd.h>

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

int count = 0;
//zy::RWMutex s_mutex;
zy::Mutex s_mutex;

void fun1() {
    ZY_LOG_INFO(g_logger) << "name: " << zy::Thread::GetName()
                             << " this.name: " << zy::Thread::GetThis()->getName()
                             << " id: " << zy::GetThreadId()
                             << " this.id: " << zy::Thread::GetThis()->getId();    
    for (int i = 0; i < 100000; ++i) {
        //zy::RWMutex::WriteLock lock(s_mutex);
        zy::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while (true) {
        ZY_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while (true) {
        ZY_LOG_INFO(g_logger) << "======================================";
    }
}
    
int main(int argc, char** argv) {
    ZY_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("../bin/conf/log2.yml");
    zy::Config::LoadFromYaml(root);

    std::vector<zy::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        zy::Thread::ptr thr(new zy::Thread(&fun2, "name_" + std::to_string(i * 2)));
        //zy::Thread::ptr thr2(new zy::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        //thrs.push_back(thr2);
    }
    
    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    ZY_LOG_INFO(g_logger) << "thread test end";
    ZY_LOG_INFO(g_logger) << "count=" << count;

    zy::Config::Visit([](zy::ConfigVarBase::ptr var) {
        ZY_LOG_INFO(g_logger) << "name=" << var->getName()
                << " description=" << var->getDescription()
                << " typename=" << var->getTypeName()
                << " value=" << var->toString();
    });
    return 0;
}