#ifndef __ZY_NONCOPYABLE_H__
#define __ZY_NONCOPYABLE_H__

#include <limits>
namespace zy {

    const uint32_t INVALID_TID = std::numeric_limits<uint32_t>::max();

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