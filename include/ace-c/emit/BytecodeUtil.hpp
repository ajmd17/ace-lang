#ifndef BYTECODE_UTIL_HPP
#define BYTECODE_UTIL_HPP

#include <memory>
#include <sstream>
#include <vector>
#include <cstdint>

// fwd declarations
struct BytecodeChunk;
struct BuildParams;
struct Buildable;

class BytecodeUtil {
public:
    template<typename T, typename... Args>
    static std::unique_ptr<T> Make(Args&&... args)
    {
        static_assert(std::is_base_of<Buildable, T>::value, "Must be a Buildable type.");
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    static std::vector<std::uint8_t> GenerateBytes(BytecodeChunk *chunk);
    static std::vector<std::uint8_t> GenerateBytes(BytecodeChunk *chunk, BuildParams &build_params);

    static std::unique_ptr<BytecodeChunk> LoadSerialized(std::istream &is);
    static void StoreSerialized(std::ostream &os, const std::unique_ptr<BytecodeChunk> &chunk);
};

#endif