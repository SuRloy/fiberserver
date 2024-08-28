#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#include "../zy/tcp_server.h"

using namespace zy;



class ChatServer : public TCPServer
{
public:
    using ptr = std::shared_ptr<ChatServer>;

    // 初始化聊天服务器对象
    explicit ChatServer(const std::string &name = "ChatServer",
                        Reactor *acceptor = Reactor::GetThis(), Reactor *worker = Reactor::GetThis());
    ~ChatServer() override;
protected:
    void handleClient(const Socket::ptr &client) override;
};

#endif //__CHATSERVER_H__