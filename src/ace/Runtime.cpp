#include <ace/Runtime.hpp>

#include <stdio.h>

namespace ace {

const int Runtime::VERSION_MAJOR = 0;
const int Runtime::VERSION_MINOR = 2;
const int Runtime::VERSION_PATCH = 0;

const char *Runtime::OS_NAME = 
#ifdef _WIN32
    "Windows";
#elif __APPLE__
    "Mac";
#elif __linux__
    "Linux";
#else
    "Unknown";
#endif

std::vector<Library> Runtime::libs = {};

vm::NativeFunctionPtr_t Library::GetFunction(const char *name)
{
    if (void *ptr = LOAD_LIB_FUNC(handle, name)) {
        return (vm::NativeFunctionPtr_t)ptr;
    }

    return nullptr;
}

Library Runtime::Load(const char *path)
{
    Library lib;
    lib.handle = OPEN_LIB(path);

    if (lib.handle != nullptr) {
        libs.push_back(lib);
    }

    return lib;
}

void Runtime::UnloadLibraries()
{
    for (auto it = libs.rbegin(); it != libs.rend(); it++) {
        if (it->handle != nullptr) {
            CLOSE_LIB(it->handle);
        }
    }
    libs.clear();
}

} // namespace ace
