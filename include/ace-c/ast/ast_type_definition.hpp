#ifndef AST_TYPE_DEFINITION_HPP
#define AST_TYPE_DEFINITION_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_variable_declaration.hpp>

#include <string>
#include <memory>
#include <vector>

class AstTypeDefinition : public AstStatement {
public:
    AstTypeDefinition(const std::string &name,
        const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
        const SourceLocation &location);
    virtual ~AstTypeDefinition() = default;

    inline const std::string &GetName() const { return m_name; }
    inline const std::vector<std::shared_ptr<AstVariableDeclaration>>
        &GetMembers() const { return m_members; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

protected:
    std::string m_name;
    std::vector<std::shared_ptr<AstVariableDeclaration>> m_members;
    int m_num_members;
};

#endif
