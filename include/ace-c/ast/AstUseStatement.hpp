#ifndef AST_USE_STATEMENT_HPP
#define AST_USE_STATEMENT_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>

#include <memory>
#include <string>

class AstUseStatement : public AstStatement {
public:
    AstUseStatement(const std::shared_ptr<AstModuleAccess> &target,
        const std::string &alias,
        const SourceLocation &location);
    virtual ~AstUseStatement() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

protected:
    std::shared_ptr<AstModuleAccess> m_target;
    std::string m_alias;
};

#endif