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
                LEVEL_ERROR,
                Msg_custom_error,
                m_location,
                std::string("'library' directive requires path names supplied in an array (e.g 'use library [\"...\"]')")
            ));
        } else {
            // find the folder which the current file is in
            std::string current_dir;

            const size_t index = m_location.GetFileName().find_last_of("/\\");
            if (index != std::string::npos) {
                current_dir = m_location.GetFileName().substr(0, index) + "/";
            }
            
            for (const std::string &path_arg : m_args) {
                // create relative path
                mod->AddScanPath(current_dir + path_arg);
            }
        }
    } else if (m_key == "strict") {

    } else {
        CompilerError error(
            LEVEL_ERROR,
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