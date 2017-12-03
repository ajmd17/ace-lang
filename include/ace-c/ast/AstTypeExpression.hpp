#ifndef AST_TYPE_EXPRESSION_HPP
#define AST_TYPE_EXPRESSION_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstTypeObject.hpp>

#include <string>
#include <memory>
#include <vector>

class AstTypeExpression : public AstExpression {
public:
    AstTypeExpression(
        const std::string &name,
        const std::shared_ptr<AstTypeSpecification> &base_specification,
        const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
        const std::vector<std::shared_ptr<AstVariableDeclaration>> &static_members,
        const SourceLocation &location);
    virtual ~AstTypeExpression() = default;

    inline const std::string &GetName() const { return m_name; }
    /** enable setting to that variable declarations can change the type name */
    inline void SetName(const std::string &name) { m_name = name; }

    inline const std::vector<std::shared_ptr<AstVariableDeclaration>>
        &GetMembers() const { return m_members; }
    inline int GetNumMembers() const { return m_num_members; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    virtual bool IsLiteral() const override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetExprType() const override;
    virtual const AstExpression *GetValueOf() const override;

protected:
    std::string m_name;
    std::shared_ptr<AstTypeSpecification> m_base_specification;
    std::vector<std::shared_ptr<AstVariableDeclaration>> m_members;
    std::vector<std::shared_ptr<AstVariableDeclaration>> m_static_members;
    int m_num_members;

    SymbolTypePtr_t m_symbol_type;

    std::shared_ptr<AstTypeObject> m_expr;

    inline Pointer<AstTypeExpression> CloneImpl() const
    {
        return Pointer<AstTypeExpression>(new AstTypeExpression(
            m_name,
            CloneAstNode(m_base_specification),
            CloneAllAstNodes(m_members),
            CloneAllAstNodes(m_static_members),
            m_location
        ));
    }
};

#endif