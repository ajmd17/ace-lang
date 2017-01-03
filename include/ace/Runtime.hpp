#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <ace-vm/Value.hpp>

#include <vector>
#include <memory>

namespace ace {

struct Library {
    void *handle;

    inline bool operator==(const Library &other) const
        { return handle == other.handle; }

    inline const void *GetHandle() const { return handle; }
    vm::NativeFunctionPtr_t GetFunction(const char *name);
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
