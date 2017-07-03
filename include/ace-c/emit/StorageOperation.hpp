#ifndef STORAGE_OPERATION_HPP
#define STORAGE_OPERATION_HPP

#include <ace-c/emit/Instruction.hpp>

#include <common/my_assert.hpp>

#include <vector>
#include <memory>

enum class Operations {
    LOAD,
    STORE,
};

enum class Methods {
    LOCAL,
    STATIC,
    ARRAY,
    MEMBER,
};

enum class Strategies {
    BY_OFFSET,
    BY_INDEX,
    BY_HASH,
};

struct StorageOperation : public Buildable {
    // fwd decls
    struct OperationBuilder;
    struct MethodBuilder;
    struct StrategyBuilder;

    StorageOperation() = default;
    virtual ~StorageOperation() = default;

    OperationBuilder GetBuilder();

    virtual size_t GetSize() const override;
    virtual void Build(Buffer &buf, BuildParams &build_params) const override;

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(op.a.reg);

        switch (method) {
            case Methods::LOCAL:
            case Methods::STATIC:

                switch (strategy) {
                    case Strategies::BY_OFFSET:
                        archive(op.b.offset);
                        
                        break;

                    case Strategies::BY_INDEX:
                        archive(op.b.index);

                        break;
                    
                    case Strategies::BY_HASH:
                        archive(op.b.hash);

                        break;
                }

                break;

            case Methods::ARRAY:
            case Methods::MEMBER:
                switch (strategy) {
                    case Strategies::BY_OFFSET:
                        ASSERT_MSG(false, "Not implemented");
                        
                        break;

                    case Strategies::BY_INDEX:
                        archive(op.b.object_data.member.index);

                        break;
                    
                    case Strategies::BY_HASH:
                        archive(op.b.object_data.member.hash);

                        break;
                }

                break;
        }
        
        archive(CEREAL_NVP(operation), CEREAL_NVP(method), CEREAL_NVP(strategy));
    }

    struct {
        union {
            RegIndex reg;
        } a;

        union {
            uint16_t index;
            uint16_t offset;
            uint32_t hash;

            struct {
                RegIndex reg;

                union {
                    RegIndex reg;
                    uint8_t index;
                    uint32_t hash;
                } member;
            } object_data;
        } b;
    } op;

    Operations operation;
    Methods method;
    Strategies strategy;

    struct StorageOperationBuilder {
        virtual ~StorageOperationBuilder() = default;
    };

    struct OperationBuilder : public StorageOperationBuilder {
        OperationBuilder(StorageOperation *op)
            : op(op)
        {
        }

        virtual ~OperationBuilder() = default;

        MethodBuilder Load(RegIndex dst);
        MethodBuilder Store(RegIndex src);

    private:
        StorageOperation *op;
    };

    struct MethodBuilder : public StorageOperationBuilder {
        MethodBuilder(StorageOperation *op, OperationBuilder *parent)
            : op(op),
              parent(parent)
        {
        }

        virtual ~MethodBuilder() = default;

        StrategyBuilder Local();
        StrategyBuilder Static();
        StrategyBuilder Array(RegIndex array_reg);
        StrategyBuilder Member(RegIndex object_reg);

        Methods method;

    private:
        StorageOperation *op;
        OperationBuilder *parent;
    };

    struct StrategyBuilder : public StorageOperationBuilder {
        StrategyBuilder(StorageOperation *op, MethodBuilder *parent)
            : op(op),
              parent(parent)
        {
        }

        void ByIndex(int index);
        void ByOffset(int offset);
        void ByHash(int hash);

        Strategies strategy;

    private:
        StorageOperation *op;
        MethodBuilder *parent;
    };
};

CEREAL_REGISTER_TYPE(StorageOperation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, StorageOperation)

#endif