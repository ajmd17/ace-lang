#include <ace-c/Configuration.hpp>

namespace ace {
namespace compiler {

const int Config::max_data_members = 255;
const std::string Config::global_module_name = "global";

const bool Config::allow_identifiers_other_modules = false;
const bool Config::use_config_file = true;

bool Config::lazy_declarations = false;

bool Config::use_static_objects = true;
bool Config::cull_unused_objects = true;

} // compiler
} // ace