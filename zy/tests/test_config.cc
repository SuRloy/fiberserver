#include "../zy/config.h"
#include "../zy/log.h"
#include <yaml-cpp/yaml.h>

zy::ConfigVar<int>::ptr g_int_value_config =
    zy::Config::Lookup("system.port", (int)8080, "system port"); 

zy::ConfigVar<float>::ptr g_float_value_config =
    zy::Config::Lookup("system.value", (float)10.2f, "system value"); 

int main (int argc, char** argv) {   
    ZY_LOG_INFO(ZY_LOG_ROOT()) << g_int_value_config->getValue();
    ZY_LOG_INFO(ZY_LOG_ROOT()) << g_float_value_config->toString();
    return 0;
}