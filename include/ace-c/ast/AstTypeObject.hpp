#ifndef AST_TYPE_OBJECT_HPP
#define AST_TYPE_OBJECT_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>

class AstTypeObject : public AstExpression {
public:
    AstTypeObject(const SymbolTypeWeakPtr_t &symbol_type,
        const std::shared_ptr<AstVariable> &proto,
        const SourceLocation &location);
    virtual ~AstTypeObject() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    SymbolTypeWeakPtr_t m_symbol_type;
    std::shared_ptr<AstVariable> m_proto;

    inline Pointer<AstTypeObject> CloneImpl() const
    {
        return Pointer<AstTypeObject>(new AstTypeObject(
            m_symbol_type,
            CloneAstNode(m_proto),
            m_location
        ));
    }
};

#endif
