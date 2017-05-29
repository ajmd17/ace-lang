#include <ace-c/ast/AstAliasDeclaration.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <iostream>

AstAliasDeclaration::AstAliasDeclaration(
    const std::string &name,
    const std::shared_ptr<AstExpression> &aliasee,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_aliasee(aliasee)
{
}

void AstAliasDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_aliasee != nullptr);
    m_aliasee->Visit(visitor, mod);

    ASSERT(m_aliasee->GetSymbolType() != nullptr);

    if (mod->LookUpIdentifier(m_name, true) != nullptr) {
        // a collision was found, add an error
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_redeclared_identifier,
            m_location,
            m_name
        ));
    } else {
        Scope &scope = mod->m_scopes.Top();
        if ((m_identifier = scope.GetIdentifierTable().AddIdentifier(m_name, FLAG_ALIAS))) {
            m_identifier->SetSymbolType(m_aliasee->GetSymbolType());
            m_identifier->SetCurrentValue(m_aliasee);
        }
    }
}

void AstAliasDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    
}

void AstAliasDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
}

void AstAliasDeclaration::Recreate(std::ostringstream &ss)
{
}

Pointer<AstStatement> AstAliasDeclaration::Clone() const
{
    return CloneImpl();
}
