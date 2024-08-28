#include "chatserver.hpp"
#include "../thirdparty/json.hpp"
#include "chatservice.hpp"
#include <functional>
#include "zy/log.h"


using namespace std;
using namespace placeholders;
using json = nlohmann::json;



ChatServer::ChatServer(const std::string &name,
                       Reactor *acceptor, Reactor *worker) : TCPServer(name, acceptor, worker)
{
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "create a new chat server, name = " << getName();

}

ChatServer::~ChatServer() {
    if (!isStop()) {
        stop();
    }
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "ChatServer stop, name = " << getName();
}

void ChatServer::handleClient(const Socket::ptr &client)
{
    string buf;
    size_t len;
    while (true)
    {
        buf.resize(4096);
        // 反序列化
        len = client->recv(&buf[0], buf.size());
        if (len > 0)
        {
            buf.resize(len);
            try
            {
                json js = json::parse(buf);
                auto msgHandler = ChatService::getInstance().getHandler(js["msgid"].get<int>());
                msgHandler(client, js);
            }
            catch (const nlohmann::json::type_error &e)
            {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Type error: " << e.what();
                // 根据需要采取行动，例如默认处理或记录错误
            }
            catch (const std::exception &e)
            {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Exception: " << e.what();
            }
        }
        else if (len == 0)
        {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << "client " << client->getPeerAddress()->toString() << " is gone";
            // 处理连接关闭或错误情况
            // TODO:改为shutdown实现优雅关闭
            ChatService::getInstance().clientCloseException(client);
            client->close();
            break;
            // 或者执行其他清理操作
        }
        else
        {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "recv() failed";
        }
    }
    //ZY_LOG_INFO(ZY_LOG_ROOT()) << "handleClient end";
}