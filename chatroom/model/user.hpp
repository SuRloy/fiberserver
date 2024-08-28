#ifndef __USER_H__
#define __USER_H__

#include <string>

class User
{
public:
    User(int id = -1, std::string name = "", 
        std::string pwd = "", std::string state = "offline")
        : id_(id),
          name_(name),
          state_(state)
    {}

    void setId(const int &id) { id_ = id; }
    void setName(const std::string &name) { name_ = name; }
    void setPassword(const std::string &paw) { password_ = paw; }
    void setState(const std::string &state) { state_ = state; }

    int getId() { return id_; }
    std::string getName() { return name_; } 
    std::string getPassword() { return password_; } 
    std::string getState() { return state_; } 

protected:
    int id_;
    std::string name_;
    std::string password_;
    std::string state_;    
};


#endif // USER_H