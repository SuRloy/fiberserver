#include "scheduler.h"
#include "log.h"

namespace zy {

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) {

}
Scheduler::~Scheduler() {

}


Scheduler* Scheduler::Getthis() {

}
Fiber* Scheduler::GetMainFiber() {

}

void Scheduler::start() {

}
void Scheduler::stop() {

}

}