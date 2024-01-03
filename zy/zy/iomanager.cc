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
        ZY_ASSERT(rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;//ET边缘触发
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        ZY_ASSERT(rt);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        ZY_ASSERT(rt);

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


}