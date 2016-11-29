#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

namespace ace {
namespace compiler {

struct Config {
    static bool lazy_declarations;
    static bool use_static_objects;
    static bool cull_unused_objects;
    static int max_registers;
};

} // compiler
} // ace

#endif
