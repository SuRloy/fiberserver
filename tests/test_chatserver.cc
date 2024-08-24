#include "../chatroom/chatserver.hpp"
#include "zy/log.h"

void run() {
    TCPServer::ptr server(new ChatServer("server"));
    Address::ptr addr = IPv4Address::Create("127.0.0.1", 12345);
    if (!addr) {
        ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "create addr failed!";
    }
    if (!server->bind(addr)) {
        ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "bind addr failed!";
    }
    server->start();
//    server->stop();
}

int main() {
    Reactor r("reactor");
    r.addTask(run);
    return 0;
}