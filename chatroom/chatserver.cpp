#include "chatserver.hpp"
#include "../thirdparty/json.hpp"
#include "chatservice.hpp"
#include <functional>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;


void ChatServer::handleClient(const Socket::ptr &client) {
    string buf;
    size_t len;
    while (true) {
        buf.resize(4096);
        //反序列化
        len = client->recv(&buf[0], buf.size());
        if (len > 0) {
            buf.resize(len);
            json js = json::parse(buf);

            auto msgHandler = ChatService::getInstance().getHandler(js["msgid"].get<int>());
            msgHandler(client, js);
        } else {
            // 处理连接关闭或错误情况
            client->close();
            // 或者执行其他清理操作
        }
    }

}