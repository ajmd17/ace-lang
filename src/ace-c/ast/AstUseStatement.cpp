#include <ace-c/ast/AstUseStatement.hpp>
#include <ace-c/ast/AstIdentifier.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/CompilerError.hpp>

#include <common/my_assert.hpp>

AstUseStatement::AstUseStatement(const std::shared_ptr<AstModuleAccess> &target,
    const std::string &alias,
    const SourceLocation &location)
    : AstStatement(location),
      m_target(target),
      m_alias(alias)
{
}

void AstUseStatement::Visit(AstVisitor *visitor, Module *mod)
{
    if (m_target) {
        // if user has created an alias name
        bool user_alias_name = m_alias.empty();

        // if module access is chained together
        bool mod_chain = false;
        Module *found_mod = mod;

        // check possible 'use' types
        AstExpression *target = m_target.get();

        while (true) {
            ASSERT(target != nullptr);

            if (AstModuleAccess *mod_access = dynamic_cast<AstModuleAccess*>(target)) {
                mod_access->PerformLookup(visitor, found_mod);
                if (mod_access->GetModule()) {
                    // set module to be the looked up module
                    found_mod = mod_access->GetModule();
                    target = mod_access->GetExpression().get();

					// set chained flag for next module access
					if (mod_chain) {
						mod_access->SetChained(true);
					}
					mod_chain = true;

                    // module access, continue
                } else {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_unknown_module, 
                            mod_access->GetLocation(), mod_access->GetTarget()));
                    
                    // module not found, cannot continue
                    break;
                }
            } else if (AstIdentifier *identifier = dynamic_cast<AstIdentifier*>(target)) {
                identifier->Visit(visitor, found_mod);

                if (identifier->GetProperties().GetIdentifierType() == IDENTIFIER_TYPE_NOT_FOUND) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_undeclared_identifier, 
                            identifier->GetLocation(), identifier->GetName()));
                } else {
                    if (!user_alias_name) {
                        m_alias = identifier->GetName();
                    }

                    // make sure the alias does not already exist
                    IdentifierLookupResult lookup_result = SemanticAnalyzer::LookupIdentifier(visitor, mod, m_alias);

                    switch (identifier->GetProperties().GetIdentifierType()) {
                        case IDENTIFIER_TYPE_VARIABLE:
                            // create variable alias
                            
                            break;
                        case IDENTIFIER_TYPE_MODULE:
                            // import module into local scope

                            break;
                        case IDENTIFIER_TYPE_TYPE:
                            // create type alias

                            break;
                    }
                }

                // finished
                break;
            } else {
                // unsupported alias type
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_unrecognized_alias_type, target->GetLocation()));

                // unknown alias type, cannot continue
                break;
            }
        }
    }
}

void AstUseStatement::Build(AstVisitor *visitor, Module *mod)
{

}

void AstUseStatement::Optimize(AstVisitor *visitor, Module *mod)
{

}

void AstUseStatement::Recreate(std::ostringstream &ss)
{

}

Pointer<AstStatement> AstUseStatement::Clone() const
{
    return CloneImpl();
}
