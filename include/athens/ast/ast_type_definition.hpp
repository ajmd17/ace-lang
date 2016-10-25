#ifndef AST_TYPE_DEFINITION_HPP
#define AST_TYPE_DEFINITION_HPP

#include <athens/ast/ast_statement.hpp>
#include <athens/ast/ast_declaration.hpp>

#include <string>
#include <memory>
#include <vector>

class AstTypeDefinition : public AstStatement {
public:
    AstTypeDefinition(const std::string &name,
        const std::vector<std::shared_ptr<AstDeclaration>> &members,
        const SourceLocation &location);
    virtual ~AstTypeDefinition() = default;

    inline const std::string &GetName() const { return m_name; }
    inline const std::vector<std::shared_ptr<AstDeclaration>> &GetMembers() const { return m_members; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

protected:
    std::string m_name;
    std::vector<std::shared_ptr<AstDeclaration>> m_members;
};

#endif
