#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/ast_visitor.hpp>

#include <common/my_assert.hpp>

AstModuleDeclaration::AstModuleDeclaration(const std::string &name, const SourceLocation &location)
    : AstDeclaration(name, location),
      m_this_module(nullptr)
{
}

void AstModuleDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    // make sure this module was not already declared/imported
    if (visitor->GetCompilationUnit()->LookupModule(m_name)) {
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_module_already_defined, m_location, m_name));
    } else {
        // add this module to the compilation unit
        visitor->GetCompilationUnit()->m_module_tree.Open(
            std::shared_ptr<Module>(new Module(m_name, m_location)));

        // TODO: we need another way to do the module index stuff,
        // it's currently linear and won't work for nested modules.
        // we'll have to do something like with my Tree template class

        // increment module index to enter the new module
        visitor->GetCompilationUnit()->m_module_index++;

        // update current module
        m_this_module = visitor->GetCompilationUnit()->GetCurrentModule().get();
        mod = m_this_module;

        // visit all children
        for (auto &child : m_children) {
            if (child) {
                child->Visit(visitor, mod);
            }
        }

        // decrement the index to refer to the previous module
        visitor->GetCompilationUnit()->m_module_index--;

        // close this module
        visitor->GetCompilationUnit()->m_module_tree.Close();
    }
}

void AstModuleDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    // increment module index to enter the new module
    visitor->GetCompilationUnit()->m_module_index++;

    ASSERT(m_this_module != nullptr);

    // build all children
    for (auto &child : m_children) {
        if (child) {
            child->Build(visitor, m_this_module);
        }
    }

    // decrement the index to refer to the previous module
    visitor->GetCompilationUnit()->m_module_index--;
}

void AstModuleDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    // increment module index to enter the new module
    visitor->GetCompilationUnit()->m_module_index++;

    ASSERT(m_this_module != nullptr);

    // optimize all children
    for (auto &child : m_children) {
        if (child) {
            child->Optimize(visitor, m_this_module);
        }
    }

    // decrement the index to refer to the previous module
    visitor->GetCompilationUnit()->m_module_index--;
}
