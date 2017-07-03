#ifndef AEX_GENERATOR_HPP
#define AEX_GENERATOR_HPP

#include <ace-c/emit/BuildableVisitor.hpp>

#include <vector>
#include <map>

class AEXGenerator : public BuildableVisitor {
public:
    AEXGenerator(Buffer &buf, BuildParams &build_params);
    virtual ~AEXGenerator() = default;

    virtual void Visit(BytecodeChunk *);
    virtual void Visit(LabelMarker *);
    virtual void Visit(Jump *);
    virtual void Visit(Comparison *);
    virtual void Visit(FunctionCall *);
    virtual void Visit(Return *);
    virtual void Visit(StoreLocal *);
    virtual void Visit(PopLocal *);
    virtual void Visit(ConstI32 *);
    virtual void Visit(ConstI64 *);
    virtual void Visit(ConstF32 *);
    virtual void Visit(ConstF64 *);
    virtual void Visit(ConstBool *);
    virtual void Visit(ConstNull *);
    virtual void Visit(BuildableTryCatch *);
    virtual void Visit(BuildableFunction *);
    virtual void Visit(BuildableType *);
    virtual void Visit(BuildableString *);
    virtual void Visit(StorageOperation *);
    virtual void Visit(RawOperation<> *);

private:
    Buffer &buf;
    BuildParams &build_params;
};

#endif