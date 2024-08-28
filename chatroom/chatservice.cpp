#include "chatservice.hpp"
#include "public.hpp"
#include "../zy/log.h"

using namespace std;
using namespace zy;

ChatService::ChatService()
{
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2)});
    msgHandlerMap_.insert({REGISTER_MSG, std::bind(&ChatService::reg, this, _1, _2)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChatHandler, this, _1, _2)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriendHandler, this, _1, _2)});

    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2)});

    msgHandlerMap_.insert({LOGINOUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2)});
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
    // ZY_LOG_INFO(ZY_LOG_ROOT()) << "do login service!";
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

            // 查询该用户是否有离线消息
            vector<string> vec = offlineMsgModel_.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，将该用户离线消息删除掉
                offlineMsgModel_.remove(id);
            }
            vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty())
            {
                vector<string> vec;
                for (auto& user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec.push_back(js.dump());
                }
                response["friends"] = vec;
            }

            // 查询用户的群组消息
            vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group& group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser& user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.emplace_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.emplace_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }


            string s = response.dump();
            client->send(s.c_str(), s.length());
        }
    }
    else
    {
        // 该用户不存在登录失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "wrong id or password";
        // 注册已经失败，不需要在json返回id
        string s = response.dump();
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
        client->send(s.c_str(), s.length());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        // 注册已经失败，不需要在json返回id
        string s = response.dump();
        client->send(s.c_str(), s.length());
    }
}

void ChatService::loginOut(const Socket::ptr &client, json &js)
{
    int userid = js["id"].get<int>();

    {
        Mutex::Lock lock(clientMutex_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    //_redis.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

void ChatService::oneChatHandler(const Socket::ptr &client, json &js)
{
    // 需要接收信息的用户ID
    int toId = js["toid"].get<int>();

    {
        Mutex::Lock lock(clientMutex_);
        auto it = userConnMap_.find(toId);
        // 确认是在线状态
        if (it != userConnMap_.end())
        {
            // TcpConnection::send() 直接发送消息
            string s = js.dump();
            it->second->send(s.c_str(), s.length());
            return;
        }
    }

    // 用户在其他主机的情况，publish消息到redis
    // User user = _userModel.query(toId);
    // if (user.getState() == "online")
    // {
    //     _redis.publish(toId, js.dump());
    //     return;
    // }

    // toId 不在线则存储离线消息
    string s = js.dump();
    offlineMsgModel_.insert(toId, s);
}

void ChatService::clientCloseException(const Socket::ptr &client)
{
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

void ChatService::addFriendHandler(const Socket::ptr &client, json &js)
{
    int userId = js["id"].get<int>();
    int friendId = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userId, friendId);
}

// 创建群组业务
void ChatService::createGroup(const Socket::ptr &client, json &js)
{
    int userId = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组消息
    Group group(-1, name, desc);
    if (groupModel_.createGroup(group))
    {
        // 存储群组创建人信息
        groupModel_.addGroup(userId, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const Socket::ptr &client, json &js)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    groupModel_.addGroup(userId, groupId, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const Socket::ptr &client, json &js)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    std::vector<int> userIdVec = groupModel_.queryGroupUsers(userId, groupId);
    string s = js.dump();

    Mutex::Lock lock(clientMutex_);
    for (int id : userIdVec)
    {
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end())
        {
            // 转发群消息
            it->second->send(s.c_str(), s.length());
        }
        else
        {
            // 查询toid是否在线
            User user = userModel_.query(id);
            if (user.getState() == "online")
            {
                // 向群组成员publish信息
                //_redis.publish(id, js.dump());
            }
            else
            {
                //转储离线消息
                offlineMsgModel_.insert(id, js.dump());
            }
        }
    }
}


void ChatService::reset()
{
    // 将所有online状态的用户，设置成offline
    userModel_.resetState();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "将所有online状态的用户,设置成offline";
}
