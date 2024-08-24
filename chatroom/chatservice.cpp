#include "chatservice.hpp"
#include "public.hpp"
#include "../zy/log.h"

using namespace std;
using namespace zy;

ChatService::ChatService()
{
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2)});
    msgHandlerMap_.insert({REGISTER_MSG, std::bind(&ChatService::reg, this, _1, _2)});
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end())
    {
        return [=](const Socket::ptr &, json &)
        {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "msgid:" << msgid << " can not find handler!";
        };
    }

    return msgHandlerMap_[msgid];
}
// 处理登录业务
void ChatService::login(const Socket::ptr &, json &)
{
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "do login service!!!";
}
// 处理注册业务
void ChatService::reg(const Socket::ptr &, json &)
{
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "do reg service!!!";
}