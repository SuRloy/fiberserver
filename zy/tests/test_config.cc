#include "../zy/config.h"
#include "../zy/log.h"
#include <yaml-cpp/yaml.h>

//默认值
zy::ConfigVar<int>::ptr g_int_value_config =
    zy::Config::Lookup("system.port", (int)8080, "system port"); 

// zy::ConfigVar<float>::ptr g_int_valuex_config =
//     zy::Config::Lookup("system.port", (float)8080, "system port"); 

zy::ConfigVar<float>::ptr g_float_value_config =
    zy::Config::Lookup("system.value", (float)10.2f, "system value"); 

zy::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    zy::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

zy::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    zy::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

zy::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    zy::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

zy::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    zy::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

zy::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    zy::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

zy::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    zy::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
            << node.Scalar() << " - " << node.Type() << " - " << level;
            //表示单个值，可以是字符串、数字、布尔值等。
    } else if (node.IsNull()) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
            << "NULL - " << node.Type() << " - " << level;
            //表示没有值或空值
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
        //表示键值对的集合，通常用于表示复杂的数据结构。在YAML中，映射使用冒号（:）表示键值对。
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
        //表示一个有序的列表，其中的元素用短横线（-）表示。
    }
}

void test_yaml() {
    //YAML::Node root = YAML::LoadFile("/home/ss/workspace/zy/bin/conf/log.yml"); 本地主机
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    print_yaml(root, 0);
}

void test_config() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            ZY_LOG_INFO(ZY_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        ZY_LOG_INFO(ZY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }
//map是pair对的读取
#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            ZY_LOG_INFO(ZY_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        ZY_LOG_INFO(ZY_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }


    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    zy::Config::LoadFromYaml(root);
    

    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);

}

class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    // bool operator==(const Person& oth) const {
    //     return m_name == oth.m_name
    //         && m_age == oth.m_age
    //         && m_sex == oth.m_sex;
    // }
};

//片特化自定义类的解析
namespace zy {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

zy::ConfigVar<Person>::ptr g_person =
    zy::Config::Lookup("class.person", Person(), "system person");

zy::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    zy::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

zy::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    zy::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
    
#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            ZY_LOG_INFO(ZY_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        ZY_LOG_INFO(ZY_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    // g_person->addListener([](const Person& old_value, const Person& new_value){
    //     ZY_LOG_INFO(ZY_LOG_ROOT()) << "old_value=" << old_value.toString()
    //             << " new_value=" << new_value.toString();
    // });

    XX_PM(g_person_map, "class.map before");
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "before: " << g_person_vec_map->toString();


    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    zy::Config::LoadFromYaml(root);

    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

int main (int argc, char** argv) {   
    // ZY_LOG_INFO(ZY_LOG_ROOT()) << g_int_value_config->getValue();
    // ZY_LOG_INFO(ZY_LOG_ROOT()) << g_float_value_config->toString();
    //test_yaml();
    //test_config();
    test_class();
    return 0;
}