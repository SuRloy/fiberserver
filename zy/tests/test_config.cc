#include "../zy/config.h"
#include "../zy/log.h"
#include <yaml-cpp/yaml.h>

zy::ConfigVar<int>::ptr g_int_value_config =
    zy::Config::Lookup("system.port", (int)8080, "system port"); 

zy::ConfigVar<float>::ptr g_float_value_config =
    zy::Config::Lookup("system.value", (float)10.2f, "system value"); 

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
            << node.Scalar() << " - " << node.Type();
    } else if (node.IsNull()) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            ZY_LOG_INFO(ZY_LOG_ROOT()) << std::string(level * 4, ' ') 
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/ss/workspace/zy/bin/conf/log.yml");
    print_yaml(root, 0);
}


int main (int argc, char** argv) {   
    ZY_LOG_INFO(ZY_LOG_ROOT()) << g_int_value_config->getValue();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << g_float_value_config->toString();
    test_yaml();
    return 0;
}