#ifndef __ZY_TIMER_H__
#define __ZY_TIMER_H__

#include <set>
#include <memory>
#include <vector>
#include <functional>
#include "utils/mutex.h"

namespace zy {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

public:
    using ptr = std::shared_ptr<Timer>;

    /**
     * @brief 定时器回调函数
     */
    using timer_callback = std::function<void()>;

    /**
     * @brief 取消定时器
     * @return 操作是否成功
     */
    bool cancel();

    /**
     * @brief 重新设置定时器的执行时间
     * @details time_ = getCurrentTime() + period
     * @return 操作是否成功
     */
    bool refresh();

    /**
     * @brief 重置定时器
     * @param period 新的周期
     * @param from_now 是否重当前时间开始计时
     * @return 操作是否成功
     */
    bool reset(uint64_t period, bool from_now);

private:
    /**
     * @brief 私有构造函数，用在 ClockManager::clocks_ 的二分查找中
     * @param time 定时器发生的时间
     */
    explicit Timer(uint64_t time);

    /**
     * @brief 私有构造函数
     * @param recurring 是否重复
     * @param period 周期
     * @param callback 定时器回调函数
     * @param manager 所属的定时器管理器
     */
    Timer(bool recurring, uint64_t period, timer_callback cb,
        TimerManager* manager);

private:
    //是否循环定时器
    bool recurring_ = false;  
    /// 执行周期
    uint64_t period_ = 0;
    //精确的执行时间
    uint64_t time_ = 0;     
    /// 定时器回调函数   
    timer_callback timer_cb_;
    /// 定时器所属的管理器
    TimerManager* manager_ = nullptr;
private:
    struct Comparator {
        bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };//比较定时器的智能指针的大小(按执行时间排序)
};

class TimerManager {
    friend class Timer;
public:
    /**
     * @brief 构造函数
     */
    TimerManager() : tickled_(false) {}

    /**
     * @brief 默认虚析构函数
     */
    virtual ~TimerManager();

    /**
     * @brief 向管理器新增一个定时器
     * @param period 周期
     * @param callback 定时器回调函数
     * @param recurring 是否重复
     * @return 新增的定时器智能指针
     */
    Timer::ptr addTimer(uint64_t period, Timer::timer_callback cb
                        ,bool recurring = false);
 
    /**
     * @brief 向管理器新增一个条件定时器
     * @param period 周期
     * @param callback 定时器回调函数
     * @param weak_cond 弱智能指针作为条件
     * @param recurring 是否重复
     * @return 新增的定时器智能指针
     */ 
    Timer::ptr addCondTimer(uint64_t period, const Timer::timer_callback& cb,
                            const std::weak_ptr<void> &weak_cond, bool recurring = false);

    /** 
     * @brief 获得距离最近发生的定时器的时间
     * @return 距离最近发生的定时器的时间
     */    
    uint64_t getNextTime();

    /**
     * @brief 列出所有超时的定时器需要执行的回调函数
     * @param callbacks 所有需要执行的回调函数
     */
    void listExpiredCb(std::vector<Timer::timer_callback>& cbs);
    
protected:
    /**
     * @brief 当插入一个定时器到堆顶时需要执行的操作
     */
    virtual void onTimerInsertAtFront() {};

    /**
     * @brief 向管理器新增一个定时器，有锁
     * @param timer 新增的定时器
     * @param lock 写锁
     */    
    void addTimer(const Timer::ptr &timer, RWMutex::WriteLock& lock);


private:
    RWMutex mutex_;
    // 定时器集合,按发生时间排列
    std::set<Timer::ptr, Timer::Comparator> timers_; 
    // 是否需要通知
    bool tickled_;           
};

}

#endif