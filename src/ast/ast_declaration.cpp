#include <athens/ast/ast_declaration.h>
#include <athens/ast_visitor.h>

AstDeclaration::AstDeclaration(const std::string &name, const SourceLocation &location)
    : AstStatement(location), 
      m_name(name),
      m_identifier(nullptr)
{
}

void AstDeclaration::Visit(AstVisitor *visitor)
{
    CompilationUnit *compilation_unit = visitor->GetCompilationUnit();
    std::unique_ptr<Module> &mod = compilation_unit->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    m_identifier = mod->LookUpIdentifier(m_name, true);
    if (m_identifier != nullptr) {
        // a collision was found, add an error
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        // add identifier
        m_identifier = scope.GetIdentifierTable().AddIdentifier(m_name);
    }
}

void AstDeclaration::Build(AstVisitor *visitor) const
{
    // get current stack size
    int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    // set identifier stack location
    m_identifier->SetStackLocation(stack_location);
    // increment stack size
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
}