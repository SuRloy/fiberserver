//线程ID和协程ID
#ifndef __ZY_UTIL_H_
#define __ZY_UTIL_H_
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace zy {

pid_t GetThreadId();
uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1); //size层数
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");
}


#endif