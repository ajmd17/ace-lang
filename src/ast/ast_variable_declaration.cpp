#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast_visitor.h>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name, 
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_assignment(assignment)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor)
{
    AstDeclaration::Visit(visitor);

    // if there was an assignment, visit it
    if (m_assignment != nullptr) {
        m_assignment->Visit(visitor);
    }
}

void AstVariableDeclaration::Build(AstVisitor *visitor) const
{
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor)
{
    if (m_assignment != nullptr) {
        m_assignment->Optimize(visitor);
    }
}