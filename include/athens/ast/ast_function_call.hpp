#ifndef AST_FUNCTION_CALL_HPP
#define AST_FUNCTION_CALL_HPP

#include <athens/ast/ast_identifier.hpp>

#include <string>
#include <vector>
#include <memory>

class AstFunctionCall : public AstIdentifier {
public:
    AstFunctionCall(const std::string &name, const std::vector<std::shared_ptr<AstExpression>> &args,
        const SourceLocation &location);
    virtual ~AstFunctionCall() = default;

    inline const std::vector<std::shared_ptr<AstExpression>>
        &GetArguments() const { return m_args; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

protected:
    std::vector<std::shared_ptr<AstExpression>> m_args;
};

#endif
