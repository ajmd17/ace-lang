#ifndef BYTECODE_CHUNK_HPP
#define BYTECODE_CHUNK_HPP

#include <ace-c/emit/Instruction.hpp>

#include <common/my_assert.hpp>

#include <vector>
#include <memory>

struct BytecodeChunk final : public Buildable {
    std::vector<LabelInfo> labels;

    BytecodeChunk() = default;
    BytecodeChunk(const BytecodeChunk &other) = delete;
    virtual ~BytecodeChunk() = default;

    inline void Append(std::unique_ptr<Buildable> buildable)
    {
        if (buildable != nullptr) {
            buildables.push_back(std::move(buildable));
        }
    }

    inline LabelId NewLabel()
    {
        LabelId index = labels.size();
        labels.emplace_back();
        return index;
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(labels), CEREAL_NVP(buildables));
    }

    std::vector<std::unique_ptr<Buildable>> buildables;

private:
    friend class BuildableVisitor;
    size_t chunk_size = 0;
};

CEREAL_REGISTER_TYPE(BytecodeChunk)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BytecodeChunk)

#endif