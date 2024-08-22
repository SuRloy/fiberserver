#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include "reactor.h"

using namespace zy;

void test_sleep() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "test_sleep begin";

    Reactor r("sleep");
    r.addTask([](){
        sleep(3);
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "test_sleep 30";
    });
    r.addTask([](){
        sleep(2);
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "test_sleep 29";
    });
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "test_sleep end";
}

void test_sock() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "36.152.44.96", &addr.sin_addr.s_addr);

    ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect begin";
    int rt = connect(fd, (const sockaddr *) &addr, sizeof addr);
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect rt = " << rt << ", err = " << strerror(errno);

    if (rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    ssize_t rt1 = send(fd, data, sizeof(data), 0);
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "send rt = " << rt1 << ", err = " << strerror(errno);

    if (rt1 <= 0) {
        return;
    }

    char buffer[4096] = {0};
    ssize_t rt2 = recv(fd, buffer, sizeof buffer, 0);
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "recv rt = " << rt2 << ", errno = " << errno;

    if (rt2 <= 0) {
        return;
    }
    buffer[rt2] = '\0';
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "recv buffer = " << buffer;
}

int main() {
//     test_sleep();

    Reactor r("socket");
    r.addTask(test_sock);

    return 0;
}