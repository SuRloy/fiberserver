#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include "reactor.h"
#include "socket.h"

using namespace zy;

void test_sock() {
    IPAddress::ptr addr = IPv4Address::Create("36.152.44.96", 80);
    if (!addr) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "create addr failed!";
    } else {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "create addr success, addr = " << addr->toString();
    }

    Socket::ptr sock = Socket::CreateTCP();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "create socket success, socket = " << sock->toString();

    int rt = sock->connect(addr);
    if (!rt) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "connect addr failed!";
    } else {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "local addr = " << sock->getLocalAddress()->toString()
                  << ", peer addr = " << sock->getPeerAddress()->toString();
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    sock->send(data, sizeof data);

    char buffer[4096] = {0};
    size_t rt2 = sock->recv(buffer, sizeof buffer);
    buffer[rt2] = '\0';
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "recv buffer = " << buffer;
}

int main(int argc, char **argv) {
    Reactor r("socket");
    r.addTask(test_sock);
    return 0;
}