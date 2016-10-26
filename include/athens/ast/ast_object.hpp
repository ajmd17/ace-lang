#ifndef AST_OBJECT_HPP
#define AST_OBJECT_HPP

#include <athens/ast/ast_expression.hpp>
#include <athens/ast/ast_declaration.hpp>

#include <memory>

class AstObject : public AstExpression {
public:
    AstObject(const ObjectType &object_type,
        const SourceLocation &location);
    virtual ~AstObject() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

private:
    ObjectType m_object_type;
};

#endif
