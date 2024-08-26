#ifndef __CHATSERVICE_H__
#define __CHATSERVICE_H__

#include <unordered_map>
#include <functional>
#include "../zy/socket.h"
#include "../thirdparty/json.hpp"
#include "usermodel.hpp"
#include "zy/utils/mutex.h"

using namespace std;
using namespace zy;
using json = nlohmann::json;
using namespace placeholders;

// 回调函数类型
using MsgHandler = std::function<void(const Socket::ptr&, json &)>;

// 聊天服务器业务类
class ChatService : NonCopyable
{
public:
    static ChatService &getInstance()
    {
        static ChatService instance_;
        return instance_;
    }
    // 处理登录业务
    void login(const Socket::ptr &client, json &js);
    // 处理注册业务
    void reg(const Socket::ptr &client, json &js);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const Socket::ptr &client);

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> msgHandlerMap_;

    //存储在线用户的通信连接
    unordered_map<int, Socket::ptr> userConnMap_;

    //数据操作类对象
    UserModel userModel_;

    //通信连接锁
    Mutex clientMutex_;
};

#endif