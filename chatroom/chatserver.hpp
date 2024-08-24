#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#include "../zy/tcp_server.h"

using namespace zy;

class ChatServer : public TCPServer
{
public:
    // 初始化聊天服务器对象
    explicit ChatServer(std::string name) : TCPServer(std::move(name)) {}

private:

    void handleClient(const Socket::ptr&) override;

};

#endif //__CHATSERVER_H__