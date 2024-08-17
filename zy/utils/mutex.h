#ifndef __ZY_MUTEX_H__
#define __ZY_MUTEX_H__

#include <pthread.h>
#include <semaphore.h>
#include "noncopyable.h"
#include <cstdint>

namespace zy {



//lock_guard
template<class T>
class ScopedLockImpl {
public:
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex), m_locked(false) {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
class ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex), m_locked(false) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
class WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex), m_locked(false) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

/**
 * @brief 互斥锁，互斥量
 */
class Mutex : NonCopyable {
public:
    using Lock = ScopedLockImpl<Mutex>;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex{};
};

/**
 * @brief  读写锁
 */
class RWMutex : NonCopyable {
public:
    using ReadLock = ReadScopedLockImpl<RWMutex>;
    using WriteLock = WriteScopedLockImpl<RWMutex>;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }


private:
    pthread_rwlock_t m_lock{};
};

/**
 * @brief 自旋锁
 * @details 一般来说，在低争用和短暂等待的情况下，使用自旋锁可能更为合适。在高争用或长时间等待的情况下，使用互斥锁可能更为合适。
            写日志的场景下使用自旋锁效率最高
            当一个线程尝试锁住已经被其他线程锁住的 spinlock 时，它并不会被阻塞，而是会一直循环（自旋）等待直到成功获得锁。
 */
class SpinLock : NonCopyable {
public:
    using Lock = ScopedLockImpl<SpinLock>;
    
    SpinLock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~SpinLock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }


private:
    pthread_spinlock_t m_mutex{};
};



//信号量
class Semaphore : NonCopyable {
public:
    explicit Semaphore(uint32_t count = 0) {
        sem_init(&sem_, 0, count);
    }

    ~Semaphore() {
        sem_destroy(&sem_);
    }

    void notify() {
        sem_post(&sem_);
    }

    void wait() {
        sem_wait(&sem_);
    }

private:
    sem_t sem_{};
};

}

#endif