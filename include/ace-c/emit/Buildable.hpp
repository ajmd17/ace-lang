#ifndef BUILDABLE_HPP
#define BUILDABLE_HPP

#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <cereal/cereal.hpp>

#include <streambuf>
#include <vector>
#include <cstdint>

using byte = uint8_t;
using Buffer = std::basic_streambuf<byte>;
using LabelPosition = uint32_t;

using Opcode = uint8_t;
using RegIndex = uint8_t;
using LabelId = size_t;

struct LabelInfo {
    LabelPosition position;

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(position));
    }
};

struct BuildParams {
    size_t block_offset = 0;
    size_t local_offset = 0;
    std::vector<LabelInfo> labels;
};

struct Buildable {
    virtual ~Buildable() = default;

    virtual size_t GetSize() const = 0;
    virtual void Build(Buffer &buf, BuildParams &build_params) const = 0;
};

#endif