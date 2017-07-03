#include <aex-builder/AEXGenerator.hpp>

#include <iostream>

class LabelVisitor : public BuildableVisitor {
public:
    LabelVisitor()
        : chunk_size(0)
    {
    }

    virtual ~LabelVisitor() = default;

    virtual void Visit(BytecodeChunk *chunk)
    {
        LabelVisitor chunk_visitor;
        chunk_visitor.chunk_offset = chunk_offset + chunk_size;

        for (auto &buildable : chunk->buildables) {
            chunk_visitor.BuildableVisitor::Visit(buildable.get());
        }

        chunk_size += chunk_visitor.chunk_size;
    }

    virtual void Visit(LabelMarker *node)
    {
        labels[node->id] = LabelInfo { (LabelPosition)(chunk_offset + chunk_size) };
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
            + sizeof(uint16_t) // len. of name
            + node->name.length() // name string
            + sizeof(uint16_t); // num. members

        for (const std::string &member_name : node->members) {
            sz += sizeof(uint16_t); // len. of member name
            sz += member_name.length(); // member name string
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

    std::map<LabelId, LabelInfo> labels;
    size_t chunk_size;
    size_t chunk_offset = 0;
};

class ChunkBuilder {
public:
    static void Build(Buffer &buf, BuildParams &params, BytecodeChunk *chunk)
    {
        std::cout << "/// begin chunk\n";
        // first, calculate labels
        LabelVisitor label_visitor;
        for (auto &buildable : chunk->buildables) {
            label_visitor.BuildableVisitor::Visit(buildable.get());
        }

        BuildParams new_params;
        new_params.block_offset = params.block_offset + params.local_offset;
        new_params.local_offset = 0;
        new_params.labels = label_visitor.labels;

        AEXGenerator chunk_generator(buf, new_params);

        for (auto &buildable : chunk->buildables) {
            chunk_generator.BuildableVisitor::Visit(buildable.get());
        }

        params.local_offset += label_visitor.chunk_size;
        std::cout << "/// end chunk\n";
    }
};

AEXGenerator::AEXGenerator(Buffer &buf, BuildParams &build_params)
    : buf(buf),
      build_params(build_params),
      m_label_visitor(new LabelVisitor)
{
}

AEXGenerator::~AEXGenerator()
{
    delete m_label_visitor;
}

void AEXGenerator::Visit(BytecodeChunk *chunk)
{
    std::cout << "/// begin chunk " << (void*)chunk << "\n";
    //ChunkBuilder::Build(buf, build_params, chunk);
    BuildParams new_params;
    new_params.block_offset = build_params.block_offset + build_params.local_offset;
    new_params.local_offset = 0;

    LabelVisitor label_visitor;
    for (auto &buildable : chunk->buildables) {
        label_visitor.BuildableVisitor::Visit(buildable.get());
    }

    new_params.labels = label_visitor.labels;

    for (auto &it : new_params.labels) {
      std::cout << "label [" << it.first << "] = " << it.second.position << "\n";
    }

    // create a new generator specifically for the chunk
    // use same output buffer
    AEXGenerator chunk_generator(buf, new_params);
    for (auto &buildable : chunk->buildables) {
        chunk_generator.BuildableVisitor::Visit(buildable.get());
    }

    build_params.local_offset += label_visitor.chunk_size;
    std::cout << "/// end chunk " << (void*)chunk << "\n";
}

void AEXGenerator::Visit(LabelMarker *node)
{
    // do nothing
}

void AEXGenerator::Visit(Jump *node)
{
    switch (node->jump_class) {
        case Jump::JumpClass::JMP:
            buf.put(Instructions::JMP);
            break;
        case Jump::JumpClass::JE:
            buf.put(Instructions::JE);
            break;
        case Jump::JumpClass::JNE:
            buf.put(Instructions::JNE);
            break;
        case Jump::JumpClass::JG:
            buf.put(Instructions::JG);
            break;
        case Jump::JumpClass::JGE:
            buf.put(Instructions::JGE);
            break;
    }

    auto it = build_params.labels.find(node->label_id);
    ASSERT(it != build_params.labels.end());

    std::cout << "BLOCK OFFSET = " << build_params.block_offset << "\n";

    LabelPosition pos = build_params.block_offset
        + it->second.position;

    buf.write((byte*)&pos, sizeof(pos));
}

void AEXGenerator::Visit(Comparison *node)
{
    switch (node->comparison_class) {
        case Comparison::ComparisonClass::CMP:
            buf.put(Instructions::CMP);
            break;
        case Comparison::ComparisonClass::CMPZ:
            buf.put(Instructions::CMPZ);
            break;
    }

    buf.put(node->reg_lhs);

    if (node->comparison_class == Comparison::ComparisonClass::CMP) {
        buf.put(node->reg_rhs);
    }
}

void AEXGenerator::Visit(FunctionCall *node)
{
    buf.put(Instructions::CALL);
    buf.put(node->reg);
    buf.put(node->nargs);
}

void AEXGenerator::Visit(Return *node)
{
    buf.put(Instructions::RET);
}

void AEXGenerator::Visit(StoreLocal *node)
{
    buf.put(Instructions::PUSH);
    buf.put(node->reg);
}

void AEXGenerator::Visit(PopLocal *node)
{
    if (node->amt > 1) {
        buf.put(Instructions::POP_N);

        byte as_byte = (byte)node->amt;
        buf.put(as_byte);
    } else {
        buf.put(Instructions::POP);
    }
}

void AEXGenerator::Visit(ConstI32 *node)
{
    buf.put(Instructions::LOAD_I32);
    buf.put(node->reg);
    buf.write((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstI64 *node)
{
    buf.put(Instructions::LOAD_I64);
    buf.put(node->reg);
    buf.write((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF32 *node)
{
    buf.put(Instructions::LOAD_F32);
    buf.put(node->reg);
    buf.write((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstF64 *node)
{
    buf.put(Instructions::LOAD_F64);
    buf.put(node->reg);
    buf.write((byte*)&node->value, sizeof(node->value));
}

void AEXGenerator::Visit(ConstBool *node)
{
    buf.put(node->value
        ? Instructions::LOAD_TRUE
        : Instructions::LOAD_FALSE);
    buf.put(node->reg);
}

void AEXGenerator::Visit(ConstNull *node)
{
    buf.put(Instructions::LOAD_NULL);
    buf.put(node->reg);
}

void AEXGenerator::Visit(BuildableTryCatch *node)
{
    buf.put(Instructions::BEGIN_TRY);

    auto it = build_params.labels.find(node->catch_label_id);
    ASSERT(it != build_params.labels.end());

    LabelPosition pos = build_params.block_offset
        + it->second.position;

    buf.write((byte*)&pos, sizeof(pos));
}

void AEXGenerator::Visit(BuildableFunction *node)
{
    auto it = build_params.labels.find(node->label_id);
    ASSERT(it != build_params.labels.end());

    LabelPosition pos = build_params.block_offset
        + it->second.position;

    // TODO: make it store and load statically
    buf.put(Instructions::LOAD_FUNC);
    buf.put(node->reg);
    buf.write((byte*)&pos, sizeof(pos));
    buf.put(node->nargs);
    buf.put(node->flags);
}

void AEXGenerator::Visit(BuildableType *node)
{
    // TODO: make it store and load statically
    buf.put(Instructions::LOAD_TYPE);
    buf.put(node->reg);

    uint16_t name_len = (uint16_t)node->name.length();
    buf.write((byte*)&name_len, sizeof(name_len));
    buf.write((byte*)&node->name[0], node->name.length());

    uint16_t size = (uint16_t)node->members.size();
    buf.write((byte*)&size, sizeof(size));

    for (const std::string &member_name : node->members) {
        uint16_t member_name_len = (uint16_t)member_name.length();
        buf.write((byte*)&member_name_len, sizeof(member_name_len));
        buf.write((byte*)&member_name[0], member_name.length());
    }
}

void AEXGenerator::Visit(BuildableString *node)
{
    uint32_t len = node->value.length();

    // TODO: make it store and load statically
    buf.put(Instructions::LOAD_STRING);
    buf.put(node->reg);
    buf.write((byte*)&len, sizeof(len));
    buf.write((byte*)&node->value[0], node->value.length());
}

void AEXGenerator::Visit(StorageOperation *node)
{
    switch (node->method) {
        case Methods::LOCAL:
            switch (node->strategy) {
                case Strategies::BY_OFFSET:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.put(Instructions::LOAD_OFFSET);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.offset, sizeof(node->op.b.offset));

                            break;
                        case Operations::STORE:
                            buf.put(Instructions::MOV_OFFSET);
                            buf.write((byte*)&node->op.b.offset, sizeof(node->op.b.offset));
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.put(Instructions::LOAD_INDEX);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.index, sizeof(node->op.b.index));

                            break;
                        case Operations::STORE:
                            buf.put(Instructions::MOV_INDEX);
                            buf.write((byte*)&node->op.b.index, sizeof(node->op.b.index));
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

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
                            buf.put(Instructions::LOAD_STATIC);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.index, sizeof(node->op.b.index));

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
                            buf.put(Instructions::LOAD_ARRAYIDX);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.put(Instructions::MOV_ARRAYIDX);
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

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
                            buf.put(Instructions::LOAD_MEM);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.put(Instructions::MOV_MEM);
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.index, sizeof(node->op.b.object_data.member.index));
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    switch (node->operation) {
                        case Operations::LOAD:
                            buf.put(Instructions::LOAD_MEM_HASH);
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));

                            break;
                        case Operations::STORE:
                            buf.put(Instructions::MOV_MEM_HASH);
                            buf.write((byte*)&node->op.b.object_data.reg, sizeof(node->op.b.object_data.reg));
                            buf.write((byte*)&node->op.b.object_data.member.hash, sizeof(node->op.b.object_data.member.hash));
                            buf.write((byte*)&node->op.a.reg, sizeof(node->op.a.reg));

                            break;
                    }

                    break;
            }

            break;
    }
}

void AEXGenerator::Visit(RawOperation<> *node)
{
    buf.put(node->opcode);
    buf.write((byte*)&node->data[0], node->data.size());
}
