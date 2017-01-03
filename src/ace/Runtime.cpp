#include <ace/Runtime.hpp>

#include <stdio.h>

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    #include <dlfcn.h>

    #define OPEN_LIB(path) dlopen((path), RTLD_LAZY)
    #define CLOSE_LIB(handle) dlclose((handle))
    #define LOAD_LIB_FUNC(handle, func) dlsym((handle), (func))
#elif _WIN32
    #include <windows.h>
    
    #define OPEN_LIB(path) LoadLibrary((path))
    #define CLOSE_LIB(handle)
    #define LOAD_LIB_FUNC(handle, func) (void*)(GetProcAddress((HMODULE)handle, func))
#endif

namespace ace {

const int Runtime::VERSION_MAJOR = 0;
const int Runtime::VERSION_MINOR = 1;
const int Runtime::VERSION_PATCH = 1;

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

    if (lib.handle) {
        libs.push_back(lib);
    }

    return lib;
}

void Runtime::UnloadLibraries()
{
    for (auto it = libs.rbegin(); it != libs.rend(); it++) {
        if ((*it).handle) {
            CLOSE_LIB((*it).handle);
        }
    }
    libs.clear();
}

} // namespace ace
