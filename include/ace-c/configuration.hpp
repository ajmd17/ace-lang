#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

namespace ace {
namespace compiler {

struct Config {
    static bool use_static_objects;
    static bool cull_unused_objects;
};

} // compiler
} // ace

#endif
