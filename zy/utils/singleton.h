#ifndef __ZY_SINGLETON_H__
#define __ZY_SINGLETON_H__

#include "noncopyable.h"

namespace zy {

/**
 * @brief 单例模式模板类
 * @tparam T 类型
 */
template<typename T>
class Singleton : NonCopyable {
public:
    static T &GetInstance() {
        static T instance_;
        return instance_;
    }
};



}

#endif