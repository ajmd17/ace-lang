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

#include <common/my_assert.hpp>

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
    
    if (m_op->GetType() & BITWISE) {
        // no bitwise operators on floats allowed.
        // do not allow right-hand side to be 'Any', because it might change the data type.
        visitor->Assert((type == ObjectType::type_builtin_int || 
            type == ObjectType::type_builtin_number || 
            type == ObjectType::type_builtin_any),
            CompilerError(Level_fatal, Msg_bitwise_operand_must_be_int, m_target->GetLocation(), type.ToString()));
    } else if (m_op->GetType() & ARITHMETIC) {
        visitor->Assert(type == ObjectType::type_builtin_int || 
            type == ObjectType::type_builtin_float || 
            type == ObjectType::type_builtin_number || 
            type == ObjectType::type_builtin_any,
            CompilerError(Level_fatal, Msg_invalid_operator_for_type, m_target->GetLocation(), m_op->ToString(), type.ToString()));
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
    m_target->Build(visitor, mod);

    if (!m_folded) {
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (m_op->GetType() == ARITHMETIC) {
            uint8_t opcode;

            if (m_op == &Operator::operator_negative) {
                opcode = NEG;
            }

            // load the label address from static memory into register 1
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(opcode, rp);

        } else if (m_op->GetType() == LOGICAL) {
            if (m_op == &Operator::operator_logical_not) {
                
                // the label to jump to the very end, and set the result to false
                StaticObject false_label;
                false_label.m_type = StaticObject::TYPE_LABEL;
                false_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                StaticObject true_label;
                true_label.m_type = StaticObject::TYPE_LABEL;
                true_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                // compare lhs to 0 (false)
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(CMPZ, rp);

                    // load the label address from static memory into register 0
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

                if (!ace::compiler::Config::use_static_objects) {
                    // fill with padding, for LOAD_ADDR instruction.
                    visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                }

                // jump if they are not equal: i.e the value is true
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JE, rp);

                    // no values were true at this point so load the value 'false'
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);

                // jump to the VERY end (so we don't load 'true' value)
                // increment register usage
                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                // get register position
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                // load the label address from static memory into register 1
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);

                if (!ace::compiler::Config::use_static_objects) {
                    // fill with padding, for LOAD_ADDR instruction.
                    visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                }

                // jump if they are equal: i.e the value is false
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JMP, rp);

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);

                // here is where the value is true
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);

                // skip to here to avoid loading 'true' into the register
                false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
            }
        }
    }
}

void AstUnaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    m_target->Optimize(visitor, mod);

    if (m_op == &Operator::operator_positive) {
        m_folded = true;
    } else {
        auto constant_value = ConstantFold(m_target, m_op, visitor);
        if (constant_value != nullptr) {
            m_target = constant_value;
            m_folded = true;
        }
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