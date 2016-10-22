#include <athens/ast/ast_function_definition.hpp>
#include <athens/ast_visitor.hpp>

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
    // TODO

    /*// open the scope for the parameters
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Open(Scope());

    // add a variable for each parameter
    for (auto &param : m_parameters) {
        param->Visit(visitor);
    }

    // close the scope for the parameters
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Close();*/

    if (m_block != nullptr) {
        // visit the function body
        m_block->Visit(visitor);
    }

    AstDeclaration::Visit(visitor);
}

void AstFunctionDefinition::Build(AstVisitor *visitor)
{
    // TODO
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor)
{
    for (auto &param : m_parameters) {
        if (param != nullptr) {
            param->Optimize(visitor);
        }
    }

    if (m_block != nullptr) {
        m_block->Optimize(visitor);
    }
}
