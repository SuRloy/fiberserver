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

uint32_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return Fiber::GetFiberId();
}

std::string GetThreadName() {
    // 系统调用要求不能超过 16 字节
    char thread_name[16];
    pthread_getname_np(pthread_self(), thread_name, 16);
    return thread_name;
}

void SetThreadName(const std::string &name) {
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
}

uint64_t GetCurrentMS() {
    struct timeval val{};
    gettimeofday(&val, nullptr);
    return val.tv_sec * 1000 + val.tv_usec / 1000;
}



void Backtrace(std::vector<std::string> &bt, int size, int skip) {
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

std::string BacktraceToString(int size, int skip, const std::string &prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);

    std::stringstream ss;
    for (const auto &i: bt) {
        ss << prefix << " " << i << std::endl;
    }
    return ss.str();
}



}
