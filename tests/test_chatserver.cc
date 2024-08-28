#include "chatroom/chatserver.hpp"
#include "chatroom/chatservice.hpp"
#include "zy/log.h"
#include <signal.h>

ChatServer::ptr server;


void run() {
    ChatServer::ptr server(new ChatServer("chatserver"));
    Address::ptr addr = IPv4Address::Create("127.0.0.1", 12345);
    if (!addr) {
        ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "create addr failed!";
    }
    if (!server->bind(addr)) {
        ZY_LOG_DEBUG(ZY_LOG_ROOT()) << "bind addr failed!";
    }
    server->start();
}

int main() {
    Reactor r("reactor", 1);
    r.addTask(run);
    return 0;
}