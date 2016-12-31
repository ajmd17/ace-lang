#include <ace-c/ast/AstDeclaration.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

AstDeclaration::AstDeclaration(const std::string &name, const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_identifier(nullptr)
{
}

void AstDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    CompilationUnit *compilation_unit = visitor->GetCompilationUnit();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine

    if ((m_identifier = mod->LookUpIdentifier(m_name, true))) {
        // a collision was found, add an error
        compilation_unit->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        if (visitor->GetCompilationUnit()->LookupModule(m_name)) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_redeclared_identifier_module, m_location, m_name));
        } else {
            // check if identifier is a type
            SymbolTypePtr_t type = mod->LookupSymbolType(m_name);

            if (type) {
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_redeclared_identifier_type, m_location, m_name));
            } else {
                // add identifier
                m_identifier = scope.GetIdentifierTable().AddIdentifier(m_name);

                TreeNode<Scope> *top = mod->m_scopes.TopNode();

                while (top) {
                    if (top->m_value.GetScopeType() == SCOPE_TYPE_FUNCTION) {
                        // set declared in function flag
                        m_identifier->GetFlags() |= FLAG_DECLARED_IN_FUNCTION;
                        break;
                    }

                    top = top->m_parent;
                }
            }
        }
    }
}