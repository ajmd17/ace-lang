#ifndef AST_OBJECT_HPP
#define AST_OBJECT_HPP

#include <ace-c/ast/AstExpression.hpp>

class AstObject : public AstExpression {
public:
    AstObject(const SymbolTypeWeakPtr_t &symbol_type, const SourceLocation &location);
    virtual ~AstObject() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    SymbolTypeWeakPtr_t m_symbol_type;

    inline Pointer<AstObject> CloneImpl() const
    {
        return Pointer<AstObject>(new AstObject(
            m_symbol_type,
            m_location
        ));
    }
};

#endif
