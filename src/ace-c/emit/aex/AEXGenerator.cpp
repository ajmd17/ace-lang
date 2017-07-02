#include <ace-c/emit/aex/AEXGenerator.hpp>

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
};

AEXGenerator::AEXGenerator(Buffer &buf, BuildParams &build_params)
    : buf(buf),
      build_params(build_params)
{
}

void AEXGenerator::Visit(BytecodeChunk *chunk)
{
    LabelVisitor label_visitor;
    label_visitor.Visit(chunk);

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
        chunk_generator.BuildableVisitor::Visit(buildable.get());
    }

    build_params.local_offset += label_visitor.chunk_size;
}

void AEXGenerator::Visit(LabelMarker *node)
{
    // do nothing
}

void AEXGenerator::Visit(Jump *node)
{
    switch (node->jump_class) {
        case Jump::JumpClass::JMP:
            buf.sputc(Instructions::JMP);
            break;
        case Jump::JumpClass::JE:
            buf.sputc(Instructions::JE);
            break;
        case Jump::JumpClass::JNE:
            buf.sputc(Instructions::JNE);
            break;
        case Jump::JumpClass::JG:
            buf.sputc(Instructions::JG);
            break;
        case Jump::JumpClass::JGE:
            buf.sputc(Instructions::JGE);
            break;
    }

    LabelPosition pos = build_params.block_offset
        + build_params.labels.at(node->label_id).position;

    buf.sputn((byte*)&pos, sizeof(pos));
}

void AEXGenerator::Visit(Comparison *node)
{
    switch (node->comparison_class) {
        case Comparison::ComparisonClass::CMP:
            buf.sputc(Instructions::CMP);
            break;
        case Comparison::ComparisonClass::CMPZ:
            buf.sputc(Instructions::CMPZ);
            break;
    }

    buf.sputc(node->reg_lhs);

    if (node->comparison_class == Comparison::ComparisonClass::CMP) {
        buf.sputc(node->reg_rhs);
    }
}

void AEXGenerator::Visit(FunctionCall *node)
{
    buf.sputc(Instructions::CALL);
    buf.sputc(node->reg);
    buf.sputc(node->nargs);
}

void AEXGenerator::Visit(Return *node)
{
    buf.sputc(Instructions::RET);
}

void AEXGenerator::Visit(StoreLocal *node)
{
    buf.sputc(Instructions::PUSH);
    buf.sputc(node->reg);
}

void AEXGenerator::Visit(PopLocal *node)
{
    if (node->amt > 1) {
        buf.sputc(Instructions::POP_N);

        byte as_byte = (byte)node->amt;
        buf.sputc(as_byte);
    } else {
        buf.sputc(Instructions::POP);
    }
}

void AEXGenerator::Visit(ConstI32 *node)
{
    buf.sputc(Instructions::LOAD_I32);
    buf.sputc(node->reg);
    buf.sputn((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstI64 *node)
{
    buf.sputc(Instructions::LOAD_I64);
    buf.sputc(node->reg);
    buf.sputn((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF32 *node)
{
    buf.sputc(Instructions::LOAD_F32);
    buf.sputc(node->reg);
    buf.sputn((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF64 *node)
{
    buf.sputc(Instructions::LOAD_F64);
    buf.sputc(node->reg);
    buf.sputn((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstBool *node)
{
    buf.sputc(node->value
        ? Instructions::LOAD_TRUE
        : Instructions::LOAD_FALSE);
    buf.sputc(node->reg);
}

void AEXGenerator::Visit(ConstNull *node)
{
    buf.sputc(Instructions::LOAD_NULL);
    buf.sputc(node->reg);
}

void AEXGenerator::Visit(BuildableTryCatch *node)
{
    buf.sputc(Instructions::BEGIN_TRY);

    LabelPosition pos = build_params.block_offset
        + build_params.labels[node->catch_label_id].position;

    buf.sputn((byte*)&pos, sizeof(pos));
}

void AEXGenerator::Visit(BuildableFunction *node)
{
    LabelPosition pos = build_params.block_offset
        + build_params.labels[node->label_id].position;

    // TODO: make it store and load statically
    buf.sputc(Instructions::LOAD_FUNC);
    buf.sputc(node->reg);
    buf.sputn((byte*)&pos, sizeof(pos));
    buf.sputc(node->nargs);
    buf.sputc(node->flags);
}

void AEXGenerator::Visit(BuildableType *node)
{
    // TODO: make it store and load statically
    buf.sputc(Instructions::LOAD_TYPE);
    buf.sputc(node->reg);

    uint16_t name_len = (uint16_t)node->name.length();
    buf.sputn((byte*)&name_len, sizeof(name_len));
    buf.sputn((byte*)&node->name[0], node->name.length());

    uint16_t size = (uint16_t)node->members.size();
    buf.sputn((byte*)&size, sizeof(size));

    for (const std::string &member_name : node->members) {
        uint16_t member_name_len = (uint16_t)member_name.length();
        buf.sputn((byte*)&member_name_len, sizeof(member_name_len));
        buf.sputn((byte*)&member_name[0], member_name.length());
    }
}

void AEXGenerator::Visit(BuildableString *node)
{
    uint32_t len = node->value.length();

    // TODO: make it store and load statically
    buf.sputc(Instructions::LOAD_STRING);
    buf.sputc(node->reg);
    buf.sputn((byte*)&len, sizeof(len));
    buf.sputn((byte*)&node->value[0], node->value.length());
}

void AEXGenerator::Visit(StorageOperation *node)
{
    switch (node->method) {
        case Methods::LOCAL:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_OFFSET);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.offset, sizeof(node->op.b.offset));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_OFFSET);
                            buf.sputn((byte*)&node->op.b.offset, sizeof(node->op.b.offset));
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_INDEX);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.index, sizeof(node->op.b.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_INDEX);
                            buf.sputn((byte*)&node->op.b.index, sizeof(node->op.b.index));
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

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
                            buf.sputc(Instructions::LOAD_STATIC);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.index, sizeof(node->op.b.index));

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
                            buf.sputc(Instructions::LOAD_ARRAYIDX);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_ARRAYIDX);
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

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
                            buf.sputc(Instructions::LOAD_MEM);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_MEM);
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_MEM_HASH);
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_MEM_HASH);
                            buf.sputn((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.sputn((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));
                            buf.sputn((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
            }

            break;
    }
}

void AEXGenerator::Visit(RawOperation<> *node)
{
    buf.sputc(node->opcode);
    buf.sputn((byte*)&node->data[0], node->data.size());
}
