#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "reactor.h"
#include "log.h"

using namespace zy;

int sockfd;
void watch_io_read();

void do_io_write() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "write callback";
    int error = 0;
    socklen_t len = sizeof error;
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (error) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect fail";
        return;
    }
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect success";

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    send(sockfd, data, sizeof(data), 0);
}

void do_io_read() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "read callback";
    char buffer[4096] = {0};
    ssize_t rt = read(sockfd, buffer, sizeof buffer);
    if (rt > 0) {
        buffer[rt] = '\0';
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "read buffer = " << buffer;
    } else if (rt == 0) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "peer close";
        close(sockfd);
        return;
    } else {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "read fail";
        close(sockfd);
        return;
    }
    // read之后重新添加读事件回调，这里不能直接调用addEvent，因为在当前位置fd的读事件上下文还是有效的，直接调用addEvent相当于重复添加相同事件
    Reactor::GetThis()->addTask(watch_io_read);
}

void test_io() {
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "36.152.44.96", &addr.sin_addr.s_addr);

    int rt = connect(sockfd, (const sockaddr *) &addr, sizeof addr);
    if (rt != 0) {
        if (errno == EINPROGRESS) {
            Reactor::GetThis()->addEvent(sockfd, zy::ReactorEvent::WRITE, do_io_write);
            Reactor::GetThis()->addEvent(sockfd, zy::ReactorEvent::READ, do_io_read);
        } else {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect error, errno = " << strerror(errno);
        }
    } else {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect return immediately";
    }
}

void watch_io_read() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "watch_io_read";
    Reactor::GetThis()->addEvent(sockfd, zy::ReactorEvent::READ, do_io_read);
}

int main() {
    Reactor r("test_reactor", 2);
    r.addTask(test_io);
    return 0;
}