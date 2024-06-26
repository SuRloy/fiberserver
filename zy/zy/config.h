#ifndef __ZY_CONFIG_H__
#define __ZY_CONFIG_H__

//将修改过东西写入配置文件，配置中心读了快速进行修改

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "thread.h"
#include "log.h"

namespace zy {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)
        ,m_description(description) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }
        virtual ~ConfigVarBase() {}

        const std::string& getName() const { return m_name;}
        const std::string& getDescription() const { return m_description;}

        //序列化和反序列化
        virtual std::string toString() = 0;//明文转换为字符串
        virtual bool fromString(const std::string& val) = 0;//解析 字符串转化为
        virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};

//定义通用类型转换模板类(F 源类型, T 目标类型)
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

//类型转换模板类片特化(YAML String 转换成 std::vector<T>)
template<class T>
class LexicalCast<std::string, std::vector<T> > {
public:
    //运用仿函数
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        //typename 关键字通常用于指示一个名称是一个类型而不是一个变量或函数
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            //递归调用
        }
        return vec;
    }
};
//类型转换模板类片特化(std::vector<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//类型转换模板类片特化(YAML String 转换成 std::list<T>)
template<class T>
class LexicalCast<std::string, std::list<T> > {
public:
    //运用仿函数
    std::list<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        //typename 关键字通常用于指示一个名称是一个类型而不是一个变量或函数
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            //递归调用
        }
        return vec;
    }
};
//类型转换模板类片特化(std::list<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//类型转换模板类片特化(YAML String 转换成 std::set<T>)
template<class T>
class LexicalCast<std::string, std::set<T> > {
public:
    //运用仿函数
    std::set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        //typename 关键字通常用于指示一个名称是一个类型而不是一个变量或函数
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
            //注意这里是insert
            //递归调用
        }
        return vec;
    }
};

//类型转换模板类片特化(std::set<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
template<class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
    //运用仿函数
    std::unordered_set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        //typename 关键字通常用于指示一个名称是一个类型而不是一个变量或函数
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
            //注意这里是insert
            //递归调用
        }
        return vec;
    }
};

//类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//类型转换模板类片特化(YAML String 转换成 std::map<T>)
template<class T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
    std::map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

//类型转换模板类片特化(std::map<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//类型转换模板类片特化(YAML String 转换成 std::unordered_map<T>)
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

//类型转换模板类片特化(std::unordered_map<T> 转换成 YAML String) 与上面是一对
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


//FromStr T operator() (const std::string&)
//ToStr std::string operator() (const T)
template<class T, class FromStr = LexicalCast<std::string, T>
                , class ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
    //通知函数原来的值和新的值
    ConfigVar(const std::string& name 
            , const T& default_value
            , const std::string& description = "")
        :ConfigVarBase(name, description)
        , m_val(default_value) {

        }

    //override c++11 让编译器检测这个函数确实从父类继承过来
    std::string toString() override{
        try {
            //类型转换 T->string
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr() (m_val);
        } catch (std::exception& e) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
                //what()返回文本错误信息
        }
        return "";
    }

    bool fromString(const std::string& val) override{
        try {
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        } catch (std::exception& e) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: string to" << typeid(m_val).name()
                << " - " << val;            
        }
        return false;
    }

    const T getValue() {
        RWMutexType::ReadLock rdlock(m_mutex);
        return m_val;
    }

    void setValue(const T& v) { 
        {//加局部域，出了这个域锁就失效了
            RWMutexType::ReadLock lock(m_mutex);
            if(v == m_val) {
                return;
            }
            for(auto& i : m_cbs) {
                i.second(m_val, v);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }

    std::string getTypeName() const override { return typeid(T).name(); }

    /**
     * @brief 添加变化回调函数
     * @return 返回该回调函数对应的唯一id,用于删除回调
     */
    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除回调函数
     * @param[in] key 回调函数的唯一id
     */
    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    /**
     * @brief 获取回调函数
     * @param[in] key 回调函数的唯一id
     * @return 如果存在返回对应的回调函数,否则返回nullptr
     */
    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 清理所有的回调函数
     */
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    RWMutexType m_mutex;
    T m_val;
    //变更回调函数组, uint64_t key,要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_value, const std::string& description = "") {
        RWMutexType::WriteLock lock(GetMutex());        
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second); 
            //std::dynamic_pointer_cast<ConfigVar<T>>(it->second); 
            if (tmp) {
                ZY_LOG_INFO(ZY_LOG_ROOT()) << "Lookup name=" << name << "exists";
                return tmp;
            } else {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Lookup name=" << name << "exists but type not "
                        << typeid(T).name() << " real_type=" << it->second->getTypeName()
                        << " " << it->second->toString();
                    return nullptr;
            }
        }        

        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "Lookup name invalid" << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }//创建

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);          
    }//查找

    static void LoadFromYaml(const YAML::Node& root);
    static ConfigVarBase::ptr LookupBase(const std::string& name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
private:
    //(Critical)静态成员在编译初始化时不知道哪个会在前面编译，会导致s_datas的内存不一定被分配出现错误
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }
    
    static RWMutexType& GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }

};

}






#endif