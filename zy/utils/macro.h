#ifndef __ZY_MACRO_H__
#define __ZY_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"
#include "../log.h"

#define ZY_ASSERT(x) \
    if (!(x)) { \
        ZY_LOG_ERROR(ZY_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << zy::backtraceToString(100, 2, "    "); \
        assert(x); \
    }

#define ZY_ASSERT2(x, w) \
    if (!(x)) { \
        ZY_LOG_ERROR(ZY_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << zy::backtraceToString(100, 2, "    "); \
        assert(x); \
    }    

#endif