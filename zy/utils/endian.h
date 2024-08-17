#ifndef __ZY_ENDIAN_H__
#define __ZY_ENDIAN_H__

#include <byteswap.h>
#include <type_traits>
#include <cstdint>

namespace zy {

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteSwap(T value) {
        return static_cast<T>(bswap_64(static_cast<uint64_t>(value)));
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteSwap(T value) {
        return static_cast<T>(bswap_32(static_cast<uint32_t>(value)));
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteSwap(T value) {
        return static_cast<T>(bswap_16(static_cast<uint16_t>(value)));
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint8_t), T>::type
    byteSwap(T value) {
        return value;
    }

#if BYTE_ORDER == BIG_ENDIAN
    template<typename T>
    T onBigEndian(T t) {
        return t;
    }

    template<typename T>
    T onLittleEndian(T t) {
        return byteSwap(t);
    }
#else
    template<typename T>
    T onBigEndian(T t) {
        return byteSwap(t);
    }

    template<typename T>
    T onLittleEndian(T t) {
        return t;
    }
#endif
}

#endif //zy_ENDIAN_H