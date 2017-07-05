#ifndef AEX_GENERATOR_HPP
#define AEX_GENERATOR_HPP

#include <aex-builder/InternalByteStream.hpp>
#include <ace-c/emit/BuildableVisitor.hpp>

#include <common/my_assert.hpp>

#include <map>
#include <sstream>
#include <vector>
#include <cstdint>

class LabelVisitor;

class AEXGenerator : public BuildableVisitor {
public:
    AEXGenerator(BuildParams &build_params);
    virtual ~AEXGenerator();

    inline InternalByteStream &GetInternalByteStream() { return m_ibs; }
    inline const InternalByteStream &GetInternalByteStream() const { return m_ibs; }

    virtual void Visit(BytecodeChunk *);

private:
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
    BuildParams &build_params;
    InternalByteStream m_ibs;
};

#endif