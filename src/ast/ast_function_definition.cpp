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
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    Identifier *result = mod->LookUpIdentifier(m_name, true);
    if (result != nullptr) {
        // a collision was found, add an error
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        // add identifier
        scope.GetIdentifierTable().AddIdentifier(m_name);

        // open the scope for the parameters
        visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Open(Scope());

        // add a variable for each parameter
        for (auto &param : m_parameters) {
            param->Visit(visitor);
        }

        // close the scope for the parameters
        visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Close();

        // visit the function body
        m_block->Visit(visitor);
    }
}

void AstFunctionDefinition::Build(AstVisitor *visitor) const
{
}

void AstFunctionDefinition::Optimize()
{
    for (auto &param : m_parameters) {
        param->Optimize();
    }

    m_block->Optimize();
}