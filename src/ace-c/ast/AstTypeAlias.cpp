#include <ace-c/ast/AstTypeAlias.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstTypeAlias::AstTypeAlias(
    const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &aliasee,
    const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_aliasee(aliasee)
{
}

void AstTypeAlias::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr && mod != nullptr);
    ASSERT(m_aliasee != nullptr);

    m_aliasee->Visit(visitor, mod);

    SymbolTypePtr_t aliasee_type = m_aliasee->GetSymbolType();
    ASSERT(aliasee_type != nullptr);

    // make sure name isn't already defined
    if (mod->LookupSymbolType(m_name)) {
        // error; redeclaration of type in module
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(
                Level_fatal,
                Msg_redefined_type,
                m_location,
                m_name
            )
        );
    } else {
        SymbolTypePtr_t alias_type = SymbolType::Alias(
            m_name, { aliasee_type }
        );

        // add it
        mod->m_scopes.Top().GetIdentifierTable().AddSymbolType(alias_type);
    }
}

void AstTypeAlias::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeAlias::Optimize(AstVisitor *visitor, Module *mod)
{
}

void AstTypeAlias::Recreate(std::ostringstream &ss)
{
    ss << "type " << m_name << " = ";
    m_aliasee->Recreate(ss);
}

Pointer<AstStatement> AstTypeAlias::Clone() const
{
    return CloneImpl();
}
