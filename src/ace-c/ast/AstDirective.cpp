#include <ace-c/ast/AstDirective.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/CompilerError.hpp>

#include <common/my_assert.hpp>

AstDirective::AstDirective(const std::string &key,
    const std::vector<std::string> &args,
    const SourceLocation &location)
    : AstStatement(location),
      m_key(key),
      m_args(args)
{
}

void AstDirective::Visit(AstVisitor *visitor, Module *mod)
{
    if (m_key == "library") {
        // library names should be supplied in arguments as all strings
        if (m_args.empty()) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                Level_fatal,
                Msg_custom_error,
                m_location,
                std::string("'library' directive requires path names supplied in an array (e.g 'use library [\"...\"]')")
            ));
        } else {
            for (const std::string &arg : m_args) {
                
            }
        }
    } else if (m_key == "strict") {

    } else {
        CompilerError error(
            Level_fatal,
            Msg_unknown_directive,
            m_location,
            m_key
        );

        visitor->GetCompilationUnit()->GetErrorList().AddError(error);
    }
}

void AstDirective::Build(AstVisitor *visitor, Module *mod)
{

}

void AstDirective::Optimize(AstVisitor *visitor, Module *mod)
{

}

void AstDirective::Recreate(std::ostringstream &ss)
{

}

Pointer<AstStatement> AstDirective::Clone() const
{
    return CloneImpl();
}
