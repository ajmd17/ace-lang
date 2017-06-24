#ifndef AIR_CODE_HPP
#define AIR_CODE_HPP

#include <common/my_assert.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <cstdint>
#include <ostream>
#include <memory>

using Opcode = uint8_t;
using LabelId = size_t;
using LabelPosition = uint32_t;
using byte = uint8_t;
using Buffer = std::basic_streambuf<byte>;

struct AIRLabel {
    LabelPosition position;

    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(position);
    }
};

struct AIRParams {
    size_t block_offset;
    size_t local_offset;
    std::vector<AIRLabel> labels;

    AIRParams() : block_offset(0), local_offset(0) {}
};

enum AIRType {
    AIR_TYPE_CHUNK,
    AIR_TYPE_JUMP
};

struct AIRBuildable {
    AIRBuildable(AIRType type) : type(type) {}

    virtual size_t GetSize() const = 0;
    virtual void Build(Buffer &buf, AIRParams &params) const = 0;

    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(type);
    }

private:
    AIRType type;
};

enum class AIRJumpClass {
    JMP,
    JE,
    JNE,
    JG,
    JGE,
};

struct AIRJump final : public AIRBuildable {
    AIRJumpClass jump_class;
    LabelId label_id;

    AIRJump() : AIRBuildable(AIR_TYPE_JUMP) {}
    AIRJump(AIRJumpClass jump_class, LabelId label_id)
        : AIRBuildable(AIR_TYPE_JUMP),
          jump_class(jump_class),
          label_id(label_id)
    {
    }

    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(cereal::virtual_base_class<AIRBuildable>(this), jump_class, label_id);
    }

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode) + sizeof(LabelPosition);
    }

    virtual void Build(Buffer &buf, AIRParams &params) const override
    {
        /*switch (jump_class) {
            case JUMP_CLASS_JMP:
                buf.sputc(JMP);
                break;
            case JUMP_CLASS_JE:
                buf.sputc(JE);
                break;
            case JUMP_CLASS_JNE:
                buf.sputc(JNE);
                break;
            case JUMP_CLASS_JG:
                buf.sputc(JG);
                break;
            case JUMP_CLASS_JGE:
                buf.sputc(JGE);
                break;
        }

        LabelPosition pos = params.block_offset
            + params.labels[label_id].position;

        buf.sputn((byte*)&pos, sizeof(pos));*/
    }
};

struct AIRCodeChunk final : public AIRBuildable {
    std::vector<AIRLabel> labels;
    size_t chunk_size;
    std::vector<std::unique_ptr<AIRBuildable>> buildables;

    AIRCodeChunk();
    //AIRCodeChunk(const std::vector<AIRLabel> &, size_t, const std::vector<std::unique_ptr<AIRBuildable>> &);
    AIRCodeChunk(const AIRCodeChunk &other) = delete;
    
    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(cereal::virtual_base_class<AIRBuildable>(this), labels, chunk_size, buildables);
    }

    inline void Append(std::unique_ptr<AIRBuildable> buildable)
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
    virtual void Build(Buffer &buf, AIRParams &params) const override;

private:
};

CEREAL_REGISTER_TYPE(AIRJump)
CEREAL_REGISTER_TYPE(AIRCodeChunk)

CEREAL_REGISTER_POLYMORPHIC_RELATION(AIRBuildable, AIRJump)
CEREAL_REGISTER_POLYMORPHIC_RELATION(AIRBuildable, AIRCodeChunk)

class AIR {
public:
    static void Store(std::ostream &os, const std::unique_ptr<AIRCodeChunk> &chunk);
    static std::unique_ptr<AIRCodeChunk> Load(std::istream &os);
};

#endif