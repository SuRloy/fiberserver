#ifndef __ZY_SINGLETON_H__
#define __ZY_SINGLETON_H__

#include "noncopyable.h"

namespace zy {
/**
 * @brief 单例模式模板类
 * @tparam T 类型
 */
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

template<class T, class X = void, int N = 0>
class singleton {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};



}

#endif