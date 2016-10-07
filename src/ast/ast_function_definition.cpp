#include <athens/ast/ast_function_definition.h>
#include <athens/ast_visitor.h>

AstFunctionDefinition::AstFunctionDefinition(const std::string &name,
    const std::vector<std::shared_ptr<AstParameter>> &parameters,
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_parameters(parameters),
      m_block(block)
{
}

void AstFunctionDefinition::Visit(AstVisitor *visitor)
{
    AstDeclaration::Visit(visitor);

    /*// open the scope for the parameters
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Open(Scope());

    // add a variable for each parameter
    for (auto &param : m_parameters) {
        param->Visit(visitor);
    }

    // close the scope for the parameters
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Close();*/

    // visit the function body
    m_block->Visit(visitor);
}

void AstFunctionDefinition::Build(AstVisitor *visitor) const
{
    AstDeclaration::Build(visitor);
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor)
{
    for (auto &param : m_parameters) {
        param->Optimize(visitor);
    }

    m_block->Optimize(visitor);
}