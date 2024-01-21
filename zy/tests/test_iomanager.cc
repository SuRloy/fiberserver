#include "zy/zy.h"
#include "zy/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

zy::Logger::ptr g_logger = ZY_LOG_ROOT();
int sock = 0;

void test_fiber() {
    ZY_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "172.18.0.1", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        ZY_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        zy::IOManager::GetThis()->addEvent(sock, zy::IOManager::READ, [](){
            ZY_LOG_INFO(g_logger) << "read callback";
        });  
        zy::IOManager::GetThis()->addEvent(sock, zy::IOManager::WRITE, [](){
            ZY_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            zy::IOManager::GetThis()->cancelEvent(sock, zy::IOManager::READ);
            close(sock);
        });    
    } else {
        ZY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}


void test01() {
    zy::IOManager iom(2, false);
    iom.schedule(&test_fiber);//不加取地址符号算值传递，传递了一个副本会新建一个下协程来处理
    
}

zy::Timer::ptr s_timer;
void test_timer() {
    zy::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        ZY_LOG_INFO(g_logger) << "hello timer i=" << i;
        if (++i == 5) {
            //timer->cancel();
            s_timer->reset(2000, true);
        }
    }, true);
}

int main(int argc, char** argv) {
    test_timer();
    //test01();
    return 0;
}