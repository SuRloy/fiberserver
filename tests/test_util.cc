#include "zy/zy.h"
#include <assert.h>
#include "zy/util.h"

zy::Logger::ptr g_logger = ZY_LOG_ROOT();

void test_assert() {
    ZY_LOG_INFO(g_logger) << zy::BacktraceToString(10);
    ZY_ASSERT2(false, "abcd");
}
int main(int argc, char** argv) {
    test_assert();
    return 0;
}