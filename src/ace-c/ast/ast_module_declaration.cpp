#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/ast_visitor.hpp>

AstModuleDeclaration::AstModuleDeclaration(const std::string &name, const SourceLocation &location)
    : AstDeclaration(name, location)
{
}

void AstModuleDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    // make sure this module was not already declared/imported
    for (const std::unique_ptr<Module> &it : visitor->GetCompilationUnit()->m_modules) {
        if (it->GetName() == m_name) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_module_already_defined, m_location, m_name));

            break;
        }
    }

    // add this module to the compilation unit
    std::unique_ptr<Module> this_module(new Module(m_name, m_location));
    visitor->GetCompilationUnit()->m_modules.push_back(std::move(this_module));
}

void AstModuleDeclaration::Build(AstVisitor *visitor, Module *mod)
{
}

void AstModuleDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
}
