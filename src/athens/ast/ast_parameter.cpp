#include <athens/ast/ast_parameter.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

AstParameter::AstParameter(const std::string &name, bool is_variadic,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_is_variadic(is_variadic)
{
}

void AstParameter::Visit(AstVisitor *visitor, Module *mod)
{
    AstDeclaration::Visit(visitor, mod);
}

void AstParameter::Build(AstVisitor *visitor, Module *mod)
{
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
