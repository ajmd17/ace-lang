#ifndef AST_TYPE_DEFINITION_HPP
#define AST_TYPE_DEFINITION_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>

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
    inline int GetNumMembers() const { return m_num_members; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

protected:
    std::string m_name;
    std::vector<std::shared_ptr<AstVariableDeclaration>> m_members;
    int m_num_members;
};

#endif
