#include <athens/ast/ast_function_definition.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>
#include <athens/object_type.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <vector>
#include <cassert>

AstFunctionDefinition::AstFunctionDefinition(const std::string &name,
    const std::shared_ptr<AstFunctionExpression> &expr,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_expr(expr)
{
}

void AstFunctionDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    assert(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    AstDeclaration::Visit(visitor, mod);

    assert(m_identifier != nullptr);

    // functions are implicitly const
    m_identifier->SetFlags(FLAG_CONST);
    m_identifier->SetObjectType(m_expr->GetObjectType());
    m_identifier->SetCurrentValue(m_expr);
}

void AstFunctionDefinition::Build(AstVisitor *visitor, Module *mod)
{
    if (m_identifier->GetUseCount() > 0) {
        // get current stack size
        int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        // build function expression
        m_expr->Build(visitor, mod);

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
    m_expr->Optimize(visitor, mod);
}
