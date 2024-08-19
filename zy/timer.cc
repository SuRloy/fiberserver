#include "timer.h"
#include "utils/util.h"

namespace zy {

bool Timer::Comparator::operator() (const Timer::ptr& lhs
                            ,const Timer::ptr& rhs) const {
    if(!lhs && !rhs) {
        return false;
    }
    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }
    if(lhs->time_ < rhs->time_) {
        return true;
    }
    if(rhs->time_ < lhs->time_) {
        return false;
    }
    return lhs.get() < rhs.get();
}



bool Timer::cancel() {
    RWMutex::WriteLock lock(manager_->mutex_);
    if (timer_cb_) {
        timer_cb_ = nullptr;
        // 在set中找到自身定时器
        auto it = manager_->timers_.find(shared_from_this());
        // 找到删除
        manager_->timers_.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    RWMutex::WriteLock lock(manager_->mutex_);
    if (!timer_cb_) {
        return false;
    }
    // 在set中找到自身定时器
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    // 删除
    manager_->timers_.erase(it);
    // 更新执行时间
    time_ = getCurrentTime() + period_;
    // 重新插入，这样能按最新的时间排序
    manager_->timers_.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t period, bool from_now) {
    RWMutex::WriteLock lock(manager_->mutex_);
    if(!timer_cb_) {
        return false;
    }
    // 在set中找到自身定时器
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    // 删除定时器
    manager_->timers_.erase(it);

    // 更新数据
    period_ = period;
    time_ = from_now ? getCurrentTime() + period_ : time_;
    // 重新加入set中
    manager_->addTimer(shared_from_this(), lock);
    return true;
}

Timer::Timer(uint64_t time)
    : recurring_(false), period_(0), time_(time), timer_cb_(nullptr), manager_(nullptr) {
}

Timer::Timer(bool recurring, uint64_t period, std::function<void()> callback, TimerManager *manager)
    : recurring_(recurring), period_(period), time_(getCurrentTime() + period_)
    , timer_cb_(std::move(callback)), manager_(manager) {
}



Timer::ptr TimerManager::addTimer(uint64_t period, std::function<void()> callback, bool recurring) {
    Timer::ptr timer1(new Timer(recurring, period, std::move(callback), this));
    RWMutex::WriteLock lock(mutex_);
    addTimer(timer1, lock);
    return timer1;
}

Timer::ptr TimerManager::addCondTimer(uint64_t period, const std::function<void()>& cb,
                                        const std::weak_ptr<void> &weak_cond, bool recurring) {
    return addTimer(period, [weak_cond, cb]() {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp) {
            cb();
        }
    }, recurring);
}

uint64_t TimerManager::getNextTime() {
    RWMutex::ReadLock lock(mutex_);
    tickled_ = false;
    // 如果没有定时器，返回一个最大值
    if (timers_.empty()) {
        return ~0ull;
    }
    // 拿到第一个定时器
    const Timer::ptr& next = *timers_.begin();
    uint64_t now_ms = getCurrentTime();
    // 如果当前时间 >= 该定时器的执行时间，说明该定时器已经超时了，该执行了
    if (now_ms >= next->time_) {
        return 0;
    } else {
        // 还没超时，返回还要多久执行
        return next->time_ - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    {
        RWMutex::ReadLock lock(mutex_);
        if (timers_.empty()) {
            return;
        }
    }

    uint64_t now = getCurrentTime();
    std::vector<Timer::ptr> expired;
    RWMutex::WriteLock lock(mutex_);
    Timer::ptr timer1(new Timer(now));
    auto it = timers_.upper_bound(timer1);
    expired.insert(expired.begin(), timers_.begin(), it);

    for (auto &timer : expired) {
        cbs.push_back(timer->timer_cb_);
        timers_.erase(timer);
        if (timer->recurring_) {
            timer->time_ = now + timer->time_;
            timers_.insert(timer);
        }
    }
}

void TimerManager::addTimer(const Timer::ptr &timer, RWMutex::WriteLock &lock) {
    auto it = timers_.insert(timer).first;
    if (it == timers_.begin() && !tickled_) {
        tickled_ = true;
    }
    lock.unlock();
    if (tickled_) {
        onTimerInsertAtFront();
    }
}


}