#include <ace-c/ast/ast_parameter.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <common/my_assert.hpp>

AstParameter::AstParameter(const std::string &name, bool is_variadic,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_is_variadic(is_variadic)
{
}

void AstParameter::Visit(AstVisitor *visitor, Module *mod)
{
    AstDeclaration::Visit(visitor, mod);
    
    if (m_type_contract != nullptr) {
        m_type_contract->Visit(visitor, mod);
    }
}

void AstParameter::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_identifier != nullptr);

    // get current stack size
    int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    // set identifier stack location
    m_identifier->SetStackLocation(stack_location);

    // increment stack size
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
}

void AstParameter::Optimize(AstVisitor *visitor, Module *mod)
{
}
