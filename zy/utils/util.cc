#include "../log.h"
#include "util.h"
#include "../fiber.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <execinfo.h>
#include <sstream>
#include <iostream>

namespace zy {

uint32_t getThreadId() {
    return syscall(SYS_gettid);
}

uint32_t getFiberId() {
    return Fiber::GetFiberId();
}

uint64_t getElapseMs() {
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);            // 系统开始运行到现在的时间
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

std::string getThreadName() {
    // 系统调用要求不能超过 16 字节
    char thread_name[16];
    pthread_getname_np(pthread_self(), thread_name, 16);
    return thread_name;
}

void setThreadName(const std::string &name) {
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
}

uint64_t getCurrentTime() {
    struct timeval val{};
    gettimeofday(&val, nullptr);
    return val.tv_sec * 1000 + val.tv_usec / 1000;
}



void backtrace(std::vector<std::string> &bt, int size, int skip) {
    void **array = (void **) ::malloc(sizeof(void *) * size);//因为使用协程，协程的栈设置很小，栈内尽量存放，所以需要用堆。
    int s = ::backtrace(array, size);

    char **strings = ::backtrace_symbols(array, size);
    if (strings == nullptr) {
        std::cerr << "backtrace_symbols error" << std::endl;
    }

    for (int i = skip; i < s; ++i) {
        bt.emplace_back(strings[i]);
    }
    ::free(array);
    ::free(strings);
}

std::string backtraceToString(int size, int skip, const std::string &prefix) {
    std::vector<std::string> bt;
    backtrace(bt, size, skip);

    std::stringstream ss;
    for (const auto &i: bt) {
        ss << prefix << " " << i << std::endl;
    }
    return ss.str();
}

std::string formatTime(time_t ts, const std::string &format) {
    struct tm tm{};
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

}
