#include <ace-c/ast/AstParameter.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstParameter::AstParameter(const std::string &name, 
    const std::shared_ptr<AstTypeSpecification> &type_spec, 
    bool is_variadic,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_type_spec(type_spec),
      m_is_variadic(is_variadic)
{
}

void AstParameter::Visit(AstVisitor *visitor, Module *mod)
{
    AstDeclaration::Visit(visitor, mod);

    // params are `Any` by default
    SymbolTypePtr_t symbol_type = SymbolType::Builtin::ANY;

    if (m_type_spec) {
        m_type_spec->Visit(visitor, mod);
        symbol_type = m_type_spec->GetSymbolType();
    }

    // if variadic, then make it array of whatever type it is
    if (m_is_variadic) {
        symbol_type = SymbolType::GenericInstance(SymbolType::Builtin::VAR_ARGS,
            GenericInstanceTypeInfo{ { symbol_type } });
    }

    if (m_identifier) {
        m_identifier->SetSymbolType(symbol_type);
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

void AstParameter::Recreate(std::ostringstream &ss)
{
    ss << m_name;
    if (m_is_variadic) {
        ss << "...";
    }
}

Pointer<AstStatement> AstParameter::Clone() const
{
    return CloneImpl();
}
