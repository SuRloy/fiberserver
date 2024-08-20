//
// Created by liucxi on 2022/11/10.
//

#include "timer.h"
#include <iostream>
#include "reactor.h"

using namespace zy;

static int timeout = 1000;
static Timer::ptr s_timer;

void timer_callback() {
    std::cout << "timer callback, timeout = " << timeout << std::endl;
    timeout += 1000;
    if(timeout < 5000) {
        s_timer->reset(timeout, true);
    } else {
        s_timer->cancel();
    }
}

int main() {
    Reactor r("reactor");
    // 循环定时器
    s_timer = r.addTimer(1000, timer_callback, true);
    // 单次定时器
    r.addTimer(500, [](){
        std::cout << "500ms timeout" << std::endl;
    });
    r.addTimer(5000, [](){
        std::cout << "5000ms timeout" << std::endl;
    });

    return 0;
}
