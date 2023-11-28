#include "../zy/config.h"
#include "../zy/log.h"
#include <yaml-cpp/yaml.h>

//默认值
zy::ConfigVar<int>::ptr g_int_value_config =
    zy::Config::Lookup("system.port", (int)8080, "system port"); 

zy::ConfigVar<float>::ptr g_float_value_config =
    zy::Config::Lookup("system.value", (float)10.2f, "system value"); 

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

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    zy::Config::LoadFromYaml(root);

    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << "after: " << g_float_value_config->toString();
}


int main (int argc, char** argv) {   
    // ZY_LOG_INFO(ZY_LOG_ROOT()) << g_int_value_config->getValue();
    // ZY_LOG_INFO(ZY_LOG_ROOT()) << g_float_value_config->toString();
    //test_yaml();
    test_config();
    return 0;
}