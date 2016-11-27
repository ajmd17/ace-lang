#include <ace-c/ast/ast_unary_expression.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/operator.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>

#include <cassert>

/** Attemps to evaluate the optimized expression at compile-time. */
static std::shared_ptr<AstConstant> ConstantFold(std::shared_ptr<AstExpression> &target, 
    const Operator *oper, AstVisitor *visitor)
{
    AstConstant *target_as_constant = dynamic_cast<AstConstant*>(target.get());

    std::shared_ptr<AstConstant> result;

    if (target_as_constant != nullptr) {
        // perform operations on these constants
        if (oper == &Operator::operator_negative) {
            result = -(*target_as_constant);
        } else if (oper == &Operator::operator_positive) {
            result.reset(target_as_constant);
        }
    }

    // one or both of the sides are not a constant
    return result;
}

AstUnaryExpression::AstUnaryExpression(const std::shared_ptr<AstExpression> &target,
    const Operator *op,
    const SourceLocation &location)
    : AstExpression(location),
      m_target(target),
      m_op(op),
      m_folded(false)
{
}

void AstUnaryExpression::Visit(AstVisitor *visitor, Module *mod)
{
    m_target->Visit(visitor, mod);

    ObjectType type = m_target->GetObjectType();
    
    visitor->Assert(type == ObjectType::type_builtin_int || 
        type == ObjectType::type_builtin_float || 
        type == ObjectType::type_builtin_number || 
        type == ObjectType::type_builtin_any,
        CompilerError(Level_fatal, Msg_invalid_operator_for_type, m_target->GetLocation(), m_op->ToString(), type.ToString()));
    
    if (m_op->GetType() & BITWISE) {
        // no bitwise operators on floats allowed.
        // do not allow right-hand side to be 'Any', because it might change the data type.
        visitor->Assert((type == ObjectType::type_builtin_int || type == ObjectType::type_builtin_number || type == ObjectType::type_builtin_any),
            CompilerError(Level_fatal, Msg_bitwise_operand_must_be_int, m_target->GetLocation(), type.ToString()));
    }

    if (m_op->ModifiesValue()) {
        AstVariable *target_as_var = nullptr;
        // check member access first
        AstMemberAccess *target_as_mem = dynamic_cast<AstMemberAccess*>(m_target.get());
        if (target_as_mem != nullptr) {
            AstIdentifier *last = target_as_mem->GetLast().get();
            target_as_var = dynamic_cast<AstVariable*>(last);
        } else {
            target_as_var = dynamic_cast<AstVariable*>(m_target.get());
        }

        if (target_as_var != nullptr) {
            if (target_as_var->GetIdentifier() != nullptr) {
                // make sure we are not modifying a const
                if (target_as_var->GetIdentifier()->GetFlags() & FLAG_CONST) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_const_modified,
                            m_target->GetLocation(), target_as_var->GetName()));
                }
            }
        } else {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_cannot_modify_rvalue,
                    m_target->GetLocation()));
        }
    }
}

void AstUnaryExpression::Build(AstVisitor *visitor, Module *mod)
{
    // TODO
}

void AstUnaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    m_target->Optimize(visitor, mod);
    auto constant_value = ConstantFold(m_target, m_op, visitor);
    if (constant_value != nullptr) {
        m_target = constant_value;
        m_folded = true;
    }
}

int AstUnaryExpression::IsTrue() const
{
    if (m_folded) {
        return m_target->IsTrue();
    }
    return -1;
}

bool AstUnaryExpression::MayHaveSideEffects() const
{
    return m_target->MayHaveSideEffects();
}

ObjectType AstUnaryExpression::GetObjectType() const
{
    return m_target->GetObjectType();
}