#include <ace-c/ast/AstUnaryExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstConstant.hpp>
#include <ace-c/Operator.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

/** Attempts to evaluate the optimized expression at compile-time. */
static std::shared_ptr<AstConstant> ConstantFold(std::shared_ptr<AstExpression> &target, 
    const Operator *oper, AstVisitor *visitor)
{
    std::shared_ptr<AstConstant> result;

    if (AstConstant *target_as_constant = dynamic_cast<AstConstant*>(target.get())) {
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
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_target(target),
      m_op(op),
      m_folded(false)
{
}

void AstUnaryExpression::Visit(AstVisitor *visitor, Module *mod)
{
    m_target->Visit(visitor, mod);

    SymbolTypePtr_t type = m_target->GetSymbolType();
    
    if (m_op->GetType() & BITWISE) {
        // no bitwise operators on floats allowed.
        // do not allow right-hand side to be 'Any', because it might change the data type.
        visitor->Assert((type == SymbolType::Builtin::INT || 
            type == SymbolType::Builtin::NUMBER ||
            type == SymbolType::Builtin::ANY),
            CompilerError(LEVEL_ERROR, Msg_bitwise_operand_must_be_int, m_target->GetLocation(), type->GetName()));
    } else if (m_op->GetType() & ARITHMETIC) {
        visitor->Assert(type == SymbolType::Builtin::INT ||
            type == SymbolType::Builtin::FLOAT ||
            type == SymbolType::Builtin::NUMBER ||
            type == SymbolType::Builtin::ANY,
            CompilerError(LEVEL_ERROR, Msg_invalid_operator_for_type, m_target->GetLocation(), m_op->ToString(), type->GetName()));
    }

    if (m_op->ModifiesValue()) {
        AstVariable *target_as_var = nullptr;
        /*// check member access first
        if (AstMemberAccess *target_as_mem = dynamic_cast<AstMemberAccess*>(m_target.get())) {
            AstIdentifier *last = target_as_mem->GetLast().get();
            target_as_var = dynamic_cast<AstVariable*>(last);
        } else {*/
            target_as_var = dynamic_cast<AstVariable*>(m_target.get());
        //}

        if (target_as_var) {
            if (target_as_var->GetProperties().GetIdentifier()) {
                // make sure we are not modifying a const
                if (target_as_var->GetProperties().GetIdentifier()->GetFlags() & FLAG_CONST) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(LEVEL_ERROR, Msg_const_modified,
                            m_target->GetLocation(), target_as_var->GetName()));
                }
            }
        } else {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(LEVEL_ERROR, Msg_cannot_modify_rvalue,
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
            uint8_t opcode = 0;

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
    } else if (auto constant_value = ConstantFold(m_target, m_op, visitor)) {
        m_target = constant_value;
        m_folded = true;
    }
}

void AstUnaryExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);
    if (!m_folded) {
        ss << m_op->ToString();
    }
    m_target->Recreate(ss);
}

Pointer<AstStatement> AstUnaryExpression::Clone() const
{
    return CloneImpl();
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

SymbolTypePtr_t AstUnaryExpression::GetSymbolType() const
{
    return m_target->GetSymbolType();
}