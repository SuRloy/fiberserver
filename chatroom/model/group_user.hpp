#ifndef __GROUP_USER_H__
#define __GROUP_USER_H__

#include "user.hpp"
#include <string>

class GroupUser : public User
{
public:
    GroupUser() = default;
    void setRole(const std::string& role) { role_ = role; }
    std::string getRole() { return role_; }

private:
    std::string role_;
};


#endif // __GROUP_USER_H__