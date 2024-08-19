#ifndef __ZY_REACTOR_H__
#define __ZY_REACTOR_H__

#include <atomic>
#include "utils/noncopyable.h"
#include "scheduler.h"
#include "timer.h"

namespace zy {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    
    enum Event {
        NONE    = 0x0,
        READ    = 0x1, //EPOLLIN
        WRITE   = 0x4, //EPOLLOUT
    };

private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr; //在哪个scheduler上执行事件
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        int fd = 0;             //时间关联的句柄
        EventContext read;      //读事件
        EventContext write;     //写事件

        Event events = NONE;  //已经注册的事件
        MutexType mutex;
    };
public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    //取消事件会触发该事件
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;
    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param[in] size 容量大小
     */
    void contextResize(size_t size);
    /**
     * @brief 判断是否可以停止
     * @param[out] timeout 最近要出发的定时器事件间隔
     * @return 返回是否可以停止
     */
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;
    int m_tickleFds[2];

    // 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    // socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};


}




#endif