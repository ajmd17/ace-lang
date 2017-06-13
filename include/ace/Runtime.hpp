#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <ace-vm/Value.hpp>

#include <ace-sdk/ace-sdk.hpp>

#include <vector>
#include <memory>

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    #include <dlfcn.h>

    #define OPEN_LIB(path) dlopen((path), RTLD_LAZY)
    #define CLOSE_LIB(handle) dlclose((handle))
    #define LOAD_LIB_FUNC(handle, func) dlsym((handle), (func))

    #define ACE_PLATFORM_UNIX 1

    #if defined(__APPLE__)
        #define ACE_PLATFORM_APPLE 1
        #define ACE_DYLIB_EXT "dylib"
    #elif defined (__linux__)
        #define ACE_PLATFORM_LINUX 1
        #define ACE_DYLIB_EXT "so"
    #endif
#elif _WIN32
    #include <windows.h>
    
    #define OPEN_LIB(path) LoadLibrary((path))
    #define CLOSE_LIB(handle) 
    #define LOAD_LIB_FUNC(handle, func) (void*)(GetProcAddress((HMODULE)handle, func))

    #define ACE_PLATFORM_WIN32 1
    #define ACE_DYLIB_EXT "dll"
#endif

namespace ace {

struct Library {
    void *handle;

    inline bool operator==(const Library &other) const
        { return handle == other.handle; }

    inline const void *GetHandle() const { return handle; }
    NativeFunctionPtr_t GetFunction(const char *name);
};

class Runtime {
public:
    static const int VERSION_MAJOR;
    static const int VERSION_MINOR;
    static const int VERSION_PATCH;

    static const char *OS_NAME;
    
public:
    static Library Load(const char *path);
    static void UnloadLibraries();

private:
    static std::vector<Library> libs;
};

} // namespace ace

#endif
