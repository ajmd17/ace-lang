#ifndef BYTECODE_UTIL_HPP
#define BYTECODE_UTIL_HPP

#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>

#include <common/my_assert.hpp>

#include <vector>
#include <sstream>
#include <cstdint>

class BytecodeUtil {
public:
    template<typename T>
    static std::unique_ptr<T> Make()
    {
        static_assert(std::is_base_of<Buildable, T>::value, "Must be a Buildable type.");
        return std::unique_ptr<T>(new T());
    }

    static std::vector<std::uint8_t> GenerateBytes(BytecodeChunk *chunk)
    {
        BuildParams build_params;
        return GenerateBytes(chunk, build_params);
    }

    static std::vector<std::uint8_t> GenerateBytes(BytecodeChunk *chunk, BuildParams &build_params)
    {
        ASSERT(chunk != nullptr);

        std::basic_stringbuf<std::uint8_t> buf;
        chunk->Build(buf, build_params);

        std::vector<std::uint8_t> vec;
        vec.reserve(chunk->GetSize());
        vec.insert(
            vec.end(),
            std::istreambuf_iterator<std::uint8_t>(&buf),
            std::istreambuf_iterator<std::uint8_t>()
        );
        
        return vec;
    }
};

#endif