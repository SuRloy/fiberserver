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
// 处理登录业务 id pwd pwd
void ChatService::login(const Socket::ptr &client, json &js)
{
    //ZY_LOG_INFO(ZY_LOG_ROOT()) << "do login service!";
    int id = js["id"].get<int>();
    std::string password = js["password"];

    User user = userModel_.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不能重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            string s = response.dump();
            s += "\n";
            client->send(s.c_str(), s.length());
        }
        else
        {
            // 登录成功，更新用户状态信息 state offline => online
            {
                Mutex::Lock lock(clientMutex_);
                userConnMap_.insert({id, client});
            }

            user.setState("online");
            userModel_.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            string s = response.dump();
            s += "\n";
            client->send(s.c_str(), s.length());    
        }
    } else {
        // 该用户不存在登录失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "wrong id or password";
        // 注册已经失败，不需要在json返回id
        string s = response.dump();
        s += "\n";
        client->send(s.c_str(), s.length());        
    }
}

// 处理注册业务 name password
void ChatService::reg(const Socket::ptr &client, json &js)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = userModel_.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        // json::dump() 将序列化信息转换为std::string
        string s = response.dump();
        s += "\n";
        client->send(std::move(s).c_str(), s.length());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        // 注册已经失败，不需要在json返回id
        string s = response.dump();
        s += "\n";
        client->send(s.c_str(), s.length());
    }
}

void ChatService::clientCloseException(const Socket::ptr &client) {
    User user;
    // 互斥锁保护
    {
        Mutex::Lock lock(clientMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it)
        {
            if (it->second == client)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }


    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}