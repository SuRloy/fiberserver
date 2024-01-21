#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace zy {

static zy::Logger::ptr g_logger = ZY_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            ZY_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    ZY_ASSERT(events & event);
    // 触发该事件就将该事件从注册事件中删掉
    events = (Event)(events & ~event);
    EventContext &ctx = getContext(event);
    if (ctx.cb) {
        // 使用地址传入就会将cb的引用计数-1
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        // 使用地址传入就会将fiber的引用计数-1
        ctx.scheduler->schedule(&ctx.fiber);
    }
    // 执行完毕将协程调度器置空
    ctx.scheduler = nullptr;
    return;
}



IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name) {
        m_epfd = epoll_create(5000);
        ZY_ASSERT(m_epfd > 0);

        int rt = pipe(m_tickleFds);
        ZY_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;//ET边缘触发
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        ZY_ASSERT(!rt);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        ZY_ASSERT(!rt);

        contextResize(32);

        start();

}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        // 没有才new新的
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    // 初始化一个 FdContext
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
 	// 从 m_fdContexts 中拿到对应的 FdContext
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        // 不够就扩充
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];     
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 一个句柄一般不会重复加同一个事件， 可能是两个不同的线程在操控同一个句柄添加事件
    if (fd_ctx->events & event) {
        ZY_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                                  << ", event = " << event
                                  << ", fd_ctx.event = " << fd_ctx->events;
        ZY_ASSERT(!(fd_ctx->events & event));
    }

    // 若已经有注册的事件则为修改操作，若没有则为添加操作
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    // 设置边缘触发模式，添加原有的事件以及要注册的事件
    epevent.events = EPOLLET | fd_ctx->events | event;
    // 将fd_ctx存到data的指针中
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        ZY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                  << op << ", " << fd << ", " << epevent.events << ") :"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }

    // 等待执行的事件数量+1
    ++m_pendingEventCount;
    // 将 fd_ctx 的注册事件更新
    fd_ctx->events = (Event)(fd_ctx->events | event);
    // 获得对应事件的 EventContext
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    // EventContext的成员应该都为空
    ZY_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb);
    // 获得当前调度器
    event_ctx.scheduler = Scheduler::GetThis();
    
    // 如果有回调就执行回调，没有就执行该协程
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        ZY_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 拿到 fd 对应的 FdContext
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
	
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 若没有要删除的事件
    if (!(fd_ctx->events & event)) {
        return false;
    }
	
    // 将事件从注册事件中删除
    Event new_events = (Event)(fd_ctx->events & ~event);
    // 若还有事件则是修改，若没事件了则删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    // 水平触发模式，新的注册事件
    epevent.events = EPOLLET | new_events;
    // ptr 关联 fd_ctx
    epevent.data.ptr = fd_ctx;
	
    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        ZY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                  << op << ", " << fd << ", " << epevent.events << ") :"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
	
    // 等待执行的事件数量-1
    --m_pendingEventCount;
    // 更新事件
    fd_ctx->events = new_events;
    // 拿到对应事件的EventContext
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    // 重置EventContext
    fd_ctx->resetContext(event_ctx);
    return true;

}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
	
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }
    
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        ZY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                  << op << ", " << fd << ", " << epevent.events << ") :"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
	
    // // 触发当前事件
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;

    return true;

}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }
	
    // 删除操作
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    // 没有事件
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        ZY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                  << op << ", " << fd << ", " << epevent.events << ") :"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    
    // 有读事件执行读事件
    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
 	// 有写事件执行写事件
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    ZY_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle() {
    // 没有在执行 idel 的线程
    if (!hasIdleThread()) {
        return;
    }
    
    // 有任务来了，就往 pipe 里发送1个字节的数据，这样 epoll_wait 就会唤醒
    int rt = write(m_tickleFds[1], "T", 1);
    ZY_ASSERT(rt == 1);

}

bool IOManager::stopping(uint64_t& timeout) {
    // 获得下次任务执行的时间
    timeout = getNextTimer();
    // 定时器为空 && 等待执行的事件数量为0 && scheduler可以stop
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}



void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    // 使用智能指针托管events， 离开idle自动释放
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                              { delete[] ptr; });
	
    while (true) {
        // 下一个任务要执行的时间
        uint64_t next_timeout = 0;
        // 获得下一个执行任务的时间，并且判断是否达到停止条件
        if (stopping(next_timeout)) {
            ZY_LOG_INFO(g_logger) << "name = " << getName()
                                     << ", idle stopping exit";
            break;
        }
        
        int rt = 0;
        do {
            // 毫秒级精度
            static const int MAX_TIMEOUT = 3000;
            //如果有定时器任务
            if (next_timeout != ~0ull) {
                // 睡眠时间为next_timeout，但不超过MAX_TIMEOUT
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : (int)next_timeout;
            } else {
                // 没定时器任务就睡眠MAX_TIMEOUT
                next_timeout = MAX_TIMEOUT;
            }
            /*  
             * 阻塞在这里，但有3中情况能够唤醒epoll_wait
             * 1. 超时时间到了
             * 2. 关注的 socket 有数据来了
             * 3. 通过 tickle 往 pipe 里发数据，表明有任务来了
             */
            rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
            /* 这里就是源码 ep_poll() 中由操作系统中断返回的 EINTR
             * 需要重新尝试 epoll_Wait */
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);
        
        std::vector<std::function<void()> > cbs;
        // 获取已经超时的任务
        listExpiredCb(cbs);
		
        // 全部放到任务队列中
        if (!cbs.empty()) {
            Scheduler::schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }
		
        // 遍历已经准备好的fd
        for (int i = 0; i < rt; ++i) {
            // 从 events 中拿一个 event
            epoll_event &event = events[i];
            // 如果获得的这个信息时来自 pipe
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                // 将 pipe 发来的1个字节数据读掉
                while (read(m_tickleFds[0], &dummy, 1) == 1);
                continue;
            }
			
            // 从 ptr 中拿出 FdContext
            FdContext *fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
			/* 在源码中，注册事件时内核会自动关注POLLERR和POLLHUP */
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                // 将读写事件都加上
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
          
            int real_events = NONE;
            // 读事件好了
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            // 写事件好了
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 没事件
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            // 剩余的事件
            int left_events = (fd_ctx->events & ~real_events);
            // 如果执行完该事件还有事件则修改，若无事件则删除
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            // 更新新的事件
            event.events = EPOLLET | left_events;
	
            // 重新注册事件
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                ZY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                          << op << ", " << fd_ctx->fd << ", " << event.events << ") :"
                                          << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
			
            // 读事件好了，执行读事件
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            // 写事件好了，执行写事件
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
		
        // 执行完epoll_wait返回的事件
        // 获得当前协程
        Fiber::ptr cur = Fiber::GetThis();
        // 获得裸指针
        auto raw_ptr = cur.get();
        // 将当前idle协程指向空指针，状态为INIT
        cur.reset();

        // 执行完返回scheduler的MainFiber 继续下一轮
        raw_ptr->swapOut();
    }

}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}