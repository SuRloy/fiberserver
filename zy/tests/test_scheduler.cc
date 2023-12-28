#include "zy/zy.h"

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

int main(int argc, char** argv) {
    zy::Scheduler sc;
    sc.start();
    sc.stop();
    return 0;
}