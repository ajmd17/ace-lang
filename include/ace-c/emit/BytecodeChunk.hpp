#ifndef BYTECODE_CHUNK_HPP
#define BYTECODE_CHUNK_HPP

#include <ace-c/emit/Instruction.hpp>

#include <common/my_assert.hpp>

#include <vector>
#include <memory>

struct BytecodeChunk final : public Buildable {
    std::vector<LabelInfo> labels;

    BytecodeChunk();
    BytecodeChunk(const BytecodeChunk &other) = delete;
    virtual ~BytecodeChunk() = default;

    inline void Append(std::unique_ptr<Buildable> buildable)
    {
        if (buildable != nullptr) {
            chunk_size += buildable->GetSize();
            buildables.push_back(std::move(buildable));
        }
    }

    inline LabelId NewLabel()
    {
        LabelId index = labels.size();
        labels.emplace_back();
        return index;
    }

    inline void MarkLabel(LabelId label_id)
    {
        ASSERT(label_id < labels.size());
        labels[label_id].position = chunk_size;
    }

    virtual size_t GetSize() const override { return chunk_size; }
    virtual void Build(Buffer &buf, BuildParams &build_params) const override;

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(labels), CEREAL_NVP(buildables), CEREAL_NVP(chunk_size));
    }
    
    std::vector<std::unique_ptr<Buildable>> buildables;

private:
    size_t chunk_size;
};

CEREAL_REGISTER_TYPE(BytecodeChunk)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BytecodeChunk)

#endif