//线程ID和协程ID
#ifndef __ZY_UTIL_H_
#define __ZY_UTIL_H_

#include <cstdint>
#include <vector>
#include <string>

namespace zy {

/**
 * @brief 获得当前线程的线程 id
 * @return 线程 id
 */
uint32_t GetThreadId();

/**
 * @brief 获得当前协程的协程 id
 * @return 线程 id
 */
uint32_t GetFiberId();

/**
 * @brief 获得当前线程的线程名
 * @return 线程名
 */
std::string GetThreadName();

/**
 * @brief 设置当前线程的线程名
 * @param name 线程名
 */
void SetThreadName(const std::string &name);

/**
 * @brief 获取当前时间
 * @return 系统当前时间
 */
uint64_t GetCurrentMS();

/**
 * @brief 获取程序调用栈
 * @param bt 保存栈信息
 * @param size 栈深度
 * @param skip 忽略前 size 层信息
 */
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/**
 * @brief 将程序调用栈格式化为字符串
 * @param size 栈深度
 * @param skip 忽略前 size 层信息
 * @param prefix 前缀信息
 * @return 字符串
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

}


#endif