#ifndef __ZY_CONFIG_H__
#define __ZY_CONFIG_H__

//将修改过东西写入配置文件，配置中心读了快速进行修改

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "zy/log.h"

namespace zy {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)
        ,m_description(description) {
        }
        virtual ~ConfigVarBase() {}

        const std::string& getName() const { return m_name;}
        const std::string& getDescription() const { return m_description;}

        virtual std::string toString() = 0;//明文转换
        virtual bool fromString(const std::string& val) = 0;//解析 初始化到自己的成员
protected:
    std::string m_name;
    std::string m_description;
};

template<class T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string& name 
            ,const T& default_value
            ,const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(default_value) {

        }
    //override c++11 让编译器检测这个函数确实从父类继承过来
    std::string toString() override{
        try {
            //类型转换 T->string
            boost::lexical_cast<std::string>(m_val);
        } catch (...) {

        }
        return "";
    }
    bool fromString(const std::string& val) override{
        
    }
private:
    T m_val;
};



}






#endif