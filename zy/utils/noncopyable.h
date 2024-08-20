#ifndef __ZY_NONCOPYABLE_H__
#define __ZY_NONCOPYABLE_H__

#include <limits>
namespace zy {

    class NonCopyable {
    public:
        NonCopyable(const NonCopyable &) = delete;

        NonCopyable &operator=(const NonCopyable &) = delete;

        NonCopyable(NonCopyable &&) = delete;

        NonCopyable &operator=(NonCopyable &&) = delete;

    protected:
        NonCopyable() = default;

        ~NonCopyable() = default;
    };
}

#endif 