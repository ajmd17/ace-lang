#ifndef AST_FUNCTION_CALL_H
#define AST_FUNCTION_CALL_H

#include <athens/ast/ast_expression.h>
#include <athens/identifier.h>

#include <string>
#include <vector>
#include <memory>

class AstFunctionCall : public AstExpression {
public:
    AstFunctionCall(const std::string &name, const std::vector<std::shared_ptr<AstExpression>> &args,
        const SourceLocation &location);
    virtual ~AstFunctionCall() = default;

    inline const std::string &GetName() const { return m_name; }
    inline const std::vector<std::shared_ptr<AstExpression>>
        &GetArguments() const { return m_args; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

protected:
    std::string m_name;
    std::vector<std::shared_ptr<AstExpression>> m_args;
    Identifier *m_identifier;
};

#endif
