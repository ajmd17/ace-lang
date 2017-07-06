#ifndef AST_FLOAT_HPP
#define AST_FLOAT_HPP

#include <ace-c/ast/AstConstant.hpp>

class AstFloat : public AstConstant {
public:
    AstFloat(ace::afloat32 value,
        const SourceLocation &location);

    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool IsNumber() const override;
    virtual ace::aint32 IntValue() const override;
    virtual ace::afloat32 FloatValue() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

    virtual std::shared_ptr<AstConstant> HandleOperator(Operators op_type, AstConstant *right) const override;

private:
    ace::afloat32 m_value;

    inline Pointer<AstFloat> CloneImpl() const
    {
        return Pointer<AstFloat>(new AstFloat(
            m_value,
            m_location
        ));
    }
};

#endif
