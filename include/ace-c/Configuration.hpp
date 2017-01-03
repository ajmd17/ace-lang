#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>

namespace ace {
namespace compiler {

struct Config {
    static const int MAX_DATA_MEMBERS;
    static const std::string GLOBAL_MODULE_NAME;

    static bool lazy_declarations;
    static bool use_static_objects;
    static bool cull_unused_objects;
    static int max_registers;
};

} // compiler
} // ace

#endif
