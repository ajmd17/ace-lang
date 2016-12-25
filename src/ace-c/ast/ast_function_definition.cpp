#include <ace-c/ast/ast_function_definition.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstFunctionDefinition::AstFunctionDefinition(const std::string &name,
    const std::shared_ptr<AstFunctionExpression> &expr,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_expr(expr)
{
}

void AstFunctionDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier != nullptr) {
        // functions are implicitly const
        m_identifier->GetFlags() |= FLAG_CONST;
        m_identifier->SetObjectType(m_expr->GetObjectType());
        m_identifier->SetCurrentValue(m_expr);
    }
}

void AstFunctionDefinition::Build(AstVisitor *visitor, Module *mod)
{
    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        // increment stack size before we build the expression
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

        // build function expression
        m_expr->Build(visitor, mod);

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);
    }
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
    m_expr->Optimize(visitor, mod);
}

void AstFunctionDefinition::Recreate(std::ostringstream &ss)
{
    ss << m_name << "=";
    m_expr->Recreate(ss);
}