#include <aex-builder/AEXGenerator.hpp>
#include <ace-c/Tree.hpp>

#include <iostream>

struct ChunkInfoNode {
    std::map<LabelId, LabelInfo> labels;
    size_t chunk_size;
};

class LabelVisitor : public BuildableVisitor {
public:
    LabelVisitor()
        : chunk_size(0)
    {
    }

    virtual ~LabelVisitor() = default;

    virtual void Visit(BytecodeChunk *chunk)
    {
        for (auto &buildable : chunk->buildables) {
            BuildableVisitor::Visit(buildable.get());
        }
    }

    virtual void Visit(LabelMarker *node)
    {
        labels[node->id] = chunk_size;
    }

    virtual void Visit(Jump *node)
    {
        chunk_size += sizeof(Opcode) + sizeof(LabelPosition);
    }

    virtual void Visit(Comparison *node)
    {
        size_t sz = sizeof(Opcode)
            + sizeof(node->reg_lhs);

        if (node->comparison_class == Comparison::ComparisonClass::CMP) {
            sz += sizeof(node->reg_rhs);
        }

        chunk_size += sz;
    }

    virtual void Visit(FunctionCall *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(node->nargs);
    }

    virtual void Visit(Return *node)
    {
        chunk_size += sizeof(Opcode);
    }

    virtual void Visit(StoreLocal *node)
    {
        chunk_size += sizeof(Opcode) + sizeof(node->reg);
    }

    virtual void Visit(PopLocal *node)
    {
        size_t sz = sizeof(Opcode);

        if (node->amt > 1) {
            sz++;
        }

        chunk_size += sz;
    }

    virtual void Visit(ConstI32 *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(node->value);
    }

    virtual void Visit(ConstI64 *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(node->value);
    }

    virtual void Visit(ConstF32 *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(node->value);
    }

    virtual void Visit(ConstF64 *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(node->value);
    }

    virtual void Visit(ConstBool *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg);
    }

    virtual void Visit(ConstNull *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg);
    }

    virtual void Visit(BuildableTryCatch *node)
    {
        chunk_size += sizeof(Opcode) + sizeof(LabelPosition);
    }

    virtual void Visit(BuildableFunction *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(LabelPosition)
            + sizeof(node->nargs)
            + sizeof(node->flags);
    }

    virtual void Visit(BuildableType *node)
    {
        size_t sz = sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(uint16_t)
            + node->name.length();

        for (const std::string &member_name : node->members) {
            sz += sizeof(uint16_t);
            sz += member_name.length();
        }

        chunk_size += sz;
    }

    virtual void Visit(BuildableString *node)
    {
        chunk_size += sizeof(Opcode)
            + sizeof(node->reg)
            + sizeof(uint32_t)
            + node->value.length();
    }

    virtual void Visit(StorageOperation *node)
    {
        size_t sz = sizeof(Opcode);

        sz += sizeof(node->op.a.reg);

        switch (node->method) {
            case Methods::LOCAL:
            case Methods::STATIC:
                switch (node->strategy) {
                    case Strategies::BY_OFFSET:
                        sz += sizeof(node->op.b.offset);
                        break;

                    case Strategies::BY_INDEX:
                        sz += sizeof(node->op.b.index);
                        break;
                    
                    case Strategies::BY_HASH:
                        ASSERT_MSG(false, "Not implemented");

                        break;
                }

                break;

            case Methods::ARRAY:
            case Methods::MEMBER:
                sz += sizeof(node->op.b.object_data.reg);

                switch (node->strategy) {
                    case Strategies::BY_OFFSET:
                        ASSERT_MSG(false, "Not implemented");

                        break;

                    case Strategies::BY_INDEX:
                        sz += sizeof(node->op.b.object_data.member.index);
                        break;
                    
                    case Strategies::BY_HASH:
                        sz += sizeof(node->op.b.object_data.member.hash);
                        break;
                }

                break;
        }

        chunk_size += sz;
    }

    virtual void Visit(RawOperation<> *node)
    {
        chunk_size += sizeof(Opcode) + node->data.size();
    }

    std::map<LabelId, LabelPosition> labels;
    size_t chunk_size;
    size_t chunk_offset = 0;
};

AEXGenerator::AEXGenerator(BuildParams &build_params)
    : build_params(build_params)
{
}

AEXGenerator::~AEXGenerator()
{
}

void AEXGenerator::Visit(BytecodeChunk *chunk)
{
    BuildParams new_params;
    new_params.block_offset = build_params.block_offset + build_params.local_offset + m_ibs.GetSize();
    new_params.local_offset = 0;

    AEXGenerator chunk_generator(new_params);
    for (auto &buildable : chunk->buildables) {
        chunk_generator.BuildableVisitor::Visit(buildable.get());
    }

    // bake the chunk's byte stream
    const std::vector<std::uint8_t> &chunk_bytes = chunk_generator.GetInternalByteStream().Bake();

    // append bytes to this chunk's InternalByteStream
    m_ibs.Put(chunk_bytes.data(), chunk_bytes.size());
    build_params.local_offset += chunk_bytes.size();

    /*std::cout << "/// begin chunk " << (void*)chunk << "\n";
    //ChunkBuilder::Build(buf, build_params, chunk);
    BuildParams new_params;
    new_params.block_offset = build_params.block_offset + build_params.local_offset;
    new_params.local_offset = 0;

    // get values from map for labels
    new_params.labels.reserve(label_visitor.labels.size());
    for (const auto &it : label_visitor.labels) {
        new_params.labels.push_back(LabelInfo { it.second });
    }

    // create a new generator specifically for the chunk
    // use same output buffer
    AEXGenerator chunk_generator(buf, new_params);

    for (auto &buildable : chunk->buildables) {
        LabelVisitor tmp;
        tmp.BuildableVisitor::Visit(buildable.get());
        chunk_generator.BuildableVisitor::Visit(buildable.get());
        new_params.local_offset += tmp.chunk_size;
    }

    build_params.local_offset += label_visitor.chunk_size;
    std::cout << "/// end chunk " << (void*)chunk << "\n";*/
}

void AEXGenerator::Visit(LabelMarker *node)
{
    m_ibs.MarkLabel(node->id);
}

void AEXGenerator::Visit(Jump *node)
{
    switch (node->jump_class) {
        case Jump::JumpClass::JMP:
            m_ibs.Put(Instructions::JMP);
            break;
        case Jump::JumpClass::JE:
            m_ibs.Put(Instructions::JE);
            break;
        case Jump::JumpClass::JNE:
            m_ibs.Put(Instructions::JNE);
            break;
        case Jump::JumpClass::JG:
            m_ibs.Put(Instructions::JG);
            break;
        case Jump::JumpClass::JGE:
            m_ibs.Put(Instructions::JGE);
            break;
    }

    // tell the InternalByteStream to set this to the label position
    // when it is available
    m_ibs.AddFixup(node->label_id, build_params.block_offset);

    /*auto it = build_params.labels.find(node->label_id);
    ASSERT(it != build_params.labels.end());

    LabelPosition pos = build_params.block_offset
        + build_params.labels.at(node->label_id).position;

    m_ibs.Put((byte*)&pos, sizeof(pos));*/
}

void AEXGenerator::Visit(Comparison *node)
{
    switch (node->comparison_class) {
        case Comparison::ComparisonClass::CMP:
            m_ibs.Put(Instructions::CMP);
            break;
        case Comparison::ComparisonClass::CMPZ:
            m_ibs.Put(Instructions::CMPZ);
            break;
    }

    m_ibs.Put(node->reg_lhs);

    if (node->comparison_class == Comparison::ComparisonClass::CMP) {
        m_ibs.Put(node->reg_rhs);
    }
}

void AEXGenerator::Visit(FunctionCall *node)
{
    m_ibs.Put(Instructions::CALL);
    m_ibs.Put(node->reg);
    m_ibs.Put(node->nargs);
}

void AEXGenerator::Visit(Return *node)
{
    m_ibs.Put(Instructions::RET);
}

void AEXGenerator::Visit(StoreLocal *node)
{
    m_ibs.Put(Instructions::PUSH);
    m_ibs.Put(node->reg);
}

void AEXGenerator::Visit(PopLocal *node)
{
    if (node->amt > 1) {
        m_ibs.Put(Instructions::POP_N);

        byte as_byte = (byte)node->amt;
        m_ibs.Put(as_byte);
    } else {
        m_ibs.Put(Instructions::POP);
    }
}

void AEXGenerator::Visit(ConstI32 *node)
{
    m_ibs.Put(Instructions::LOAD_I32);
    m_ibs.Put(node->reg);
    m_ibs.Put((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstI64 *node)
{
    m_ibs.Put(Instructions::LOAD_I64);
    m_ibs.Put(node->reg);
    m_ibs.Put((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF32 *node)
{
    m_ibs.Put(Instructions::LOAD_F32);
    m_ibs.Put(node->reg);
    m_ibs.Put((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF64 *node)
{
    m_ibs.Put(Instructions::LOAD_F64);
    m_ibs.Put(node->reg);
    m_ibs.Put((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstBool *node)
{
    m_ibs.Put(node->value
        ? Instructions::LOAD_TRUE
        : Instructions::LOAD_FALSE);
    m_ibs.Put(node->reg);
}

void AEXGenerator::Visit(ConstNull *node)
{
    m_ibs.Put(Instructions::LOAD_NULL);
    m_ibs.Put(node->reg);
}

void AEXGenerator::Visit(BuildableTryCatch *node)
{
    m_ibs.Put(Instructions::BEGIN_TRY);
    m_ibs.AddFixup(node->catch_label_id, build_params.block_offset);

    /*auto it = build_params.labels.find(node->catch_label_id);
    ASSERT(it != build_params.labels.end());

    LabelPosition pos = build_params.block_offset
        + build_params.labels[node->catch_label_id].position;

    m_ibs.Put((byte*)&pos, sizeof(pos));*/
}

void AEXGenerator::Visit(BuildableFunction *node)
{
    /*auto it = build_params.labels.find(node->label_id);
    ASSERT(it != build_params.labels.end());

    LabelPosition pos = build_params.block_offset
        + it->second.position;*/

    // TODO: make it store and load statically
    m_ibs.Put(Instructions::LOAD_FUNC);
    m_ibs.Put(node->reg);
    m_ibs.AddFixup(node->label_id, build_params.block_offset);
    //m_ibs.Put((byte*)&pos, sizeof(pos));
    m_ibs.Put(node->nargs);
    m_ibs.Put(node->flags);
}

void AEXGenerator::Visit(BuildableType *node)
{
    // TODO: make it store and load statically
    m_ibs.Put(Instructions::LOAD_TYPE);
    m_ibs.Put(node->reg);

    uint16_t name_len = (uint16_t)node->name.length();
    m_ibs.Put((byte*)&name_len, sizeof(name_len));
    m_ibs.Put((byte*)&node->name[0], node->name.length());

    uint16_t size = (uint16_t)node->members.size();
    m_ibs.Put((byte*)&size, sizeof(size));

    for (const std::string &member_name : node->members) {
        uint16_t member_name_len = (uint16_t)member_name.length();
        m_ibs.Put((byte*)&member_name_len, sizeof(member_name_len));
        m_ibs.Put((byte*)&member_name[0], member_name.length());
    }
}

void AEXGenerator::Visit(BuildableString *node)
{
    uint32_t len = node->value.length();

    // TODO: make it store and load statically
    m_ibs.Put(Instructions::LOAD_STRING);
    m_ibs.Put(node->reg);
    m_ibs.Put((byte*)&len, sizeof(len));
    m_ibs.Put((byte*)&node->value[0], node->value.length());
}

void AEXGenerator::Visit(StorageOperation *node)
{
    switch (node->method) {
        case Methods::LOCAL:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_OFFSET);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.offset, sizeof(node->op.b.offset));

                            break;
                        case Operations::STORE:
                            m_ibs.Put(Instructions::MOV_OFFSET);
                            m_ibs.Put((byte*)&node->op.b.offset, sizeof(node->op.b.offset));
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_INDEX);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.index, sizeof(node->op.b.index));

                            break;
                        case Operations::STORE:
                            m_ibs.Put(Instructions::MOV_INDEX);
                            m_ibs.Put((byte*)&node->op.b.index, sizeof(node->op.b.index));
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::STATIC:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_STATIC);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.index, sizeof(node->op.b.index));

                            break;
                        case Operations::STORE:
                            ASSERT_MSG(false, "Not implemented");

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::ARRAY:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_ARRAYIDX);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            m_ibs.Put(Instructions::MOV_ARRAYIDX);
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::MEMBER:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_MEM);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            m_ibs.Put(Instructions::MOV_MEM);
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    switch (node->operation) {
                        case Operations::LOAD:
                            m_ibs.Put(Instructions::LOAD_MEM_HASH);
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));

                            break;
                        case Operations::STORE:
                            m_ibs.Put(Instructions::MOV_MEM_HASH);
                            m_ibs.Put((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            m_ibs.Put((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));
                            m_ibs.Put((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
            }

            break;
    }
}

void AEXGenerator::Visit(RawOperation<> *node)
{
    m_ibs.Put(node->opcode);
    m_ibs.Put((byte*)&node->data[0], node->data.size());
}
