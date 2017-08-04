#ifndef AST_INTEGER_HPP
#define AST_INTEGER_HPP

#include <ace-c/ast/AstConstant.hpp>

#include <cstdint>

class AstInteger : public AstConstant {
public:
    AstInteger(ace::aint32 value, const SourceLocation &location);

    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool IsNumber() const override;
    virtual ace::aint32 IntValue() const override;
    virtual ace::afloat32 FloatValue() const override;
    virtual SymbolTypePtr_t GetExprType() const override;

    virtual std::shared_ptr<AstConstant> HandleOperator(Operators op_type, AstConstant *right) const override;

private:
    ace::aint32 m_value;

    inline Pointer<AstInteger> CloneImpl() const
    {
        return Pointer<AstInteger>(new AstInteger(
            m_value,
            m_location
        ));
    }
};

#endif
