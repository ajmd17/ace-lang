#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>
#include <map>
#include <fstream>

namespace ace {
namespace compiler {

struct Config {
    static const int max_data_members;
    static const std::string global_module_name;

    /** Allow implicit usage of variables from different modules? */
    static const bool allow_identifiers_other_modules;
    /** Use aceconfig.ini in path to change compilation parameters */
    static const bool use_config_file;

    /** Allow Python-style declarations? */
    static bool lazy_declarations;
    
    /** Store strings, functions, etc.... at
        the top of the program, or load them at the point they're needed */
    static bool use_static_objects;
    /** Optimize by removing unused variables */
    static bool cull_unused_objects;
};

} // compiler
} // ace

#endif
