#include <ace-c/ast/AstTypeOfExpression.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>

AstTypeOfExpression::AstTypeOfExpression(
    const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_static_id(0)
{
}

void AstTypeOfExpression::Visit(AstVisitor *visitor, Module *mod)
{
     ASSERT(m_expr != nullptr);
     m_expr->Visit(visitor, mod);
    
     SymbolTypePtr_t expr_type = m_expr->GetSymbolType();
     ASSERT(expr_type != nullptr);

     //if (expr_type == BuiltinTypes::ANY) {
        // add runtime::typeof call
        m_runtime_typeof_call = visitor->GetCompilationUnit()->GetAstNodeBuilder()
            .Module("runtime")
            .Function("typeof")
            .Call({
                std::shared_ptr<AstArgument>((new AstArgument(
                    m_expr,
                    false,
                    "",
                    SourceLocation::eof
                )))
            });

        ASSERT(m_runtime_typeof_call != nullptr);
        m_runtime_typeof_call->Visit(visitor, mod);
    //}
}

std::unique_ptr<Buildable> AstTypeOfExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_runtime_typeof_call != nullptr);
    //if (m_runtime_typeof_call != nullptr) {
    return m_runtime_typeof_call->Build(visitor, mod);
    /*} else {
        ASSERT(m_expr != nullptr);

        SymbolTypePtr_t expr_type = m_expr->GetSymbolType();
        ASSERT(expr_type != nullptr);
        
        std::string expr_type_name = expr_type->GetName();

        // simply add a string representing the type

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
    }*/
}

void AstTypeOfExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_runtime_typeof_call != nullptr) {
        m_runtime_typeof_call->Optimize(visitor, mod);
    } else {
        ASSERT(m_expr != nullptr);
        m_expr->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstTypeOfExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstTypeOfExpression::IsTrue() const
{
    return Tribool::True();
}

bool AstTypeOfExpression::MayHaveSideEffects() const
{
    ASSERT(m_expr != nullptr);

    return m_expr->MayHaveSideEffects();
}

SymbolTypePtr_t AstTypeOfExpression::GetSymbolType() const
{
    ASSERT(m_expr != nullptr);
    
    return BuiltinTypes::STRING;
}