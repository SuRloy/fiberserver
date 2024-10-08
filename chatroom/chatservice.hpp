#ifndef __CHATSERVICE_H__
#define __CHATSERVICE_H__

#include <unordered_map>
#include <functional>
#include "../zy/socket.h"
#include "../thirdparty/json.hpp"

#include "model/usermodel.hpp"
#include "model/offlinemessagemodel.hpp"
#include "model/friendmodel.hpp"
#include "model/group_model.hpp"
#include "redis.hpp"

#include "zy/utils/mutex.h"

using namespace std;
using namespace zy;
using json = nlohmann::json;
using namespace placeholders;

// 回调函数类型
using MsgHandler = std::function<void(const Socket::ptr &, json &)>;

// 聊天服务器业务类
class ChatService : NonCopyable
{
public:
    static ChatService &getInstance()
    {
        static ChatService instance_;
        return instance_;
    }
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理登录业务
    void login(const Socket::ptr &client, json &js);
    // 处理注册业务
    void reg(const Socket::ptr &client, json &js);
    //注销服务
    void loginOut(const Socket::ptr &client, json &js);
    // 一对一聊天业务
    void oneChatHandler(const Socket::ptr &client, json &js);
    // 添加好友业务
    void addFriendHandler(const Socket::ptr &client, json &js);
    // 创建群组业务
    void createGroup(const Socket::ptr &client, json &js);
    // 加入群组业务
    void addGroup(const Socket::ptr &client, json &js);
    // 群组聊天业务
    void groupChat(const Socket::ptr &client, json &js);
    // 处理客户端异常退出
    void clientCloseException(const Socket::ptr &client);
    // 处理redis订阅信息
    void handleRedisSubscribeMessage(int userid, string msg);
    // 服务器异常，业务重置方法
    void reset();

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    unordered_map<int, Socket::ptr> userConnMap_;

    // 数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // 通信连接锁
    Mutex clientMutex_;

    //redis操作对象
    Redis redis_;
};

#endif