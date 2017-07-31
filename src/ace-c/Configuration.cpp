#include <ace-c/Configuration.hpp>

namespace ace {
namespace compiler {

const int Config::max_data_members = 255;
const std::string Config::global_module_name = "global";
bool Config::use_static_objects = true;
bool Config::cull_unused_objects = true;

} // compiler
} // ace