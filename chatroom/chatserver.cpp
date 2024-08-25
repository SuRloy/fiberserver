#include "chatserver.hpp"
#include "../thirdparty/json.hpp"
#include "chatservice.hpp"
#include <functional>
#include "zy/log.h"

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
            try {
                // 这里假设我们想用字符串键访问JSON对象
                json js = json::parse(buf);
                auto msgHandler = ChatService::getInstance().getHandler(js["msgid"].get<int>());
                msgHandler(client, js);
            } catch (const nlohmann::json::type_error& e) {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Type error: " << e.what();
                // 根据需要采取行动，例如默认处理或记录错误
            } catch (const std::exception& e) {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Exception: " << e.what();
            }
        } else {
            // 处理连接关闭或错误情况
            client->close();
            break;
            // 或者执行其他清理操作
        }
    }

}