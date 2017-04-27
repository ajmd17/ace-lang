#include <ace-c/ast/AstTypeOfExpression.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstTypeOfExpression::AstTypeOfExpression(
    const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location),
      m_expr(expr),
      m_static_id(0)
{
}

void AstTypeOfExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);

    m_expr->Visit(visitor, mod);
}

void AstTypeOfExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);

    auto expr_type = m_expr->GetSymbolType();
    ASSERT(expr_type != nullptr);
    
    std::string expr_type_name = expr_type->GetName();

    // simply add a string representing the type
    // TODO: if type is 'Any' do a runtime type check

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    StaticObject so(expr_type_name.c_str());

    int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
    if (found_id == -1) {
        m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
        so.m_id = m_static_id;
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
    } else {
        m_static_id = found_id;
    }

    // load static object into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, m_static_id);

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding for LOAD_STRING instruction
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2 + std::strlen(so.m_value.str);
    }
}

void AstTypeOfExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    
    m_expr->Optimize(visitor, mod);
}

void AstTypeOfExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_expr != nullptr);
    
    m_expr->Recreate(ss);
}

Pointer<AstStatement> AstTypeOfExpression::Clone() const
{
    return CloneImpl();
}

int AstTypeOfExpression::IsTrue() const
{
    return 1;
}

bool AstTypeOfExpression::MayHaveSideEffects() const
{
    ASSERT(m_expr != nullptr);

    return m_expr->MayHaveSideEffects();
}

SymbolTypePtr_t AstTypeOfExpression::GetSymbolType() const
{
    ASSERT(m_expr != nullptr);
    
    return SymbolType::Builtin::STRING;
}