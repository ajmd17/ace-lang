#ifndef AST_VARIABLE_HPP
#define AST_VARIABLE_HPP

#include <ace-c/ast/AstIdentifier.hpp>

class AstVariable : public AstIdentifier {
public:
    AstVariable(const std::string &name, const SourceLocation &location);
    virtual ~AstVariable() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

private:
    inline Pointer<AstVariable> CloneImpl() const
    {
        return Pointer<AstVariable>(new AstVariable(
            m_name,
            m_location));
    }
};

#endif
