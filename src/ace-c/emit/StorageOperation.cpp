#include <ace-c/emit/StorageOperation.hpp>

void StorageOperation::StrategyBuilder::ByIndex(int index)
{
    op->strategy = strategy = Strategies::BY_INDEX;

    switch (parent->method) {
        case Methods::ARRAY:
        case Methods::MEMBER:
            op->op.b.object_data.member.index = index;
            break;
        default:
            op->op.b.index = index;
            break;
    }
}

void StorageOperation::StrategyBuilder::ByOffset(int offset)
{
    op->strategy = strategy = Strategies::BY_OFFSET;
    
    switch (parent->method) {
        case Methods::ARRAY:
        case Methods::MEMBER:
            ASSERT_MSG(false, "Not implemented");
            break;
        default:
            op->op.b.offset = offset;
            break;
    }
}

void StorageOperation::StrategyBuilder::ByHash(int hash)
{
    op->strategy = strategy = Strategies::BY_HASH;
    
    switch (parent->method) {
        case Methods::ARRAY:
        case Methods::MEMBER:
            op->op.b.object_data.member.hash = hash;
            break;
        default:
            op->op.b.hash = hash;
            break;
    }
}

StorageOperation::StrategyBuilder StorageOperation::MethodBuilder::Local()
{
    op->method = method = Methods::LOCAL;

    return StrategyBuilder(op, this);
}

StorageOperation::StrategyBuilder StorageOperation::MethodBuilder::Static()
{
    op->method = method = Methods::STATIC;

    return StrategyBuilder(op, this);
}

StorageOperation::StrategyBuilder StorageOperation::MethodBuilder::Array(RegIndex array_reg)
{
    op->method = method = Methods::ARRAY;
    op->op.b.object_data.reg = array_reg;

    return StrategyBuilder(op, this);
}

StorageOperation::StrategyBuilder StorageOperation::MethodBuilder::Member(RegIndex object_reg)
{
    op->method = method = Methods::MEMBER;
    op->op.b.object_data.reg = object_reg;

    return StrategyBuilder(op, this);
}

StorageOperation::MethodBuilder StorageOperation::OperationBuilder::Load(RegIndex dst)
{
    op->operation = Operations::LOAD;
    op->op.a.reg = dst;

    return MethodBuilder(op, this);
}

StorageOperation::MethodBuilder StorageOperation::OperationBuilder::Store(RegIndex src)
{
    op->operation = Operations::STORE;
    op->op.a.reg = src;

    return MethodBuilder(op, this);
}

StorageOperation::OperationBuilder StorageOperation::GetBuilder()
{
    return OperationBuilder(this);
}

size_t StorageOperation::GetSize() const
{
    size_t sz = sizeof(Opcode);

    sz += sizeof(op.a.reg);

    switch (method) {
        case Methods::LOCAL:
        case Methods::STATIC:
            switch (strategy) {
                case Strategies::BY_OFFSET:
                    sz += sizeof(op.b.offset);
                    break;

                case Strategies::BY_INDEX:
                    sz += sizeof(op.b.index);
                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::ARRAY:
        case Methods::MEMBER:
            sz += sizeof(op.b.object_data.reg);

            switch (strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");

                    break;

                case Strategies::BY_INDEX:
                    sz += sizeof(op.b.object_data.member.index);
                    break;
                
                case Strategies::BY_HASH:
                    sz += sizeof(op.b.object_data.member.hash);
                    break;
            }

            break;
    }

    return sz;
}

void StorageOperation::Build(Buffer &buf, BuildParams &build_params) const
{
    switch (method) {
        case Methods::LOCAL:
            switch (strategy) {
                case Strategies::BY_OFFSET:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_OFFSET);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.offset, sizeof(op.b.offset));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_OFFSET);
                            buf.sputn((byte*)&op.b.offset, sizeof(op.b.offset));
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));

                            break;
                    }
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_INDEX);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.index, sizeof(op.b.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_INDEX);
                            buf.sputn((byte*)&op.b.index, sizeof(op.b.index));
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::STATIC:
            switch (strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_STATIC);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.index, sizeof(op.b.index));

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
            switch (strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_ARRAYIDX);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.index, sizeof(op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_ARRAYIDX);
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.index, sizeof(op.b.object_data.member.index));
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    ASSERT_MSG(false, "Not implemented");

                    break;
            }

            break;

        case Methods::MEMBER:
            switch (strategy) {
                case Strategies::BY_OFFSET:
                    ASSERT_MSG(false, "Not implemented");
                    
                    break;

                case Strategies::BY_INDEX:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_MEM);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.index, sizeof(op.b.object_data.member.index));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_MEM);
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.index, sizeof(op.b.object_data.member.index));
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));

                            break;
                    }

                    break;
                
                case Strategies::BY_HASH:
                    switch (operation) {
                        case Operations::LOAD:
                            buf.sputc(Instructions::LOAD_MEM_HASH);
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.hash, sizeof(op.b.object_data.member.hash));

                            break;
                        case Operations::STORE:
                            buf.sputc(Instructions::MOV_MEM_HASH);
                            buf.sputn((byte*)&op.b.object_data.reg, sizeof(op.b.object_data.reg));
                            buf.sputn((byte*)&op.b.object_data.member.hash, sizeof(op.b.object_data.member.hash));
                            buf.sputn((byte*)&op.a.reg, sizeof(op.a.reg));

                            break;
                    }

                    break;
            }

            break;
    }
}