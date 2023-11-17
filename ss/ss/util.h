//线程ID和协程ID
#ifndef __SS_UTIL_H_
#define __SS_UTIL_H_
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

namespace ss {

pid_t GetThreadId();
uint32_t GetFiberId();

}


#endif