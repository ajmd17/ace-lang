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
static std::shared_ptr<AstConstant> ConstantFold(
    std::shared_ptr<AstExpression> &target, 
    Operators op_type,
    AstVisitor *visitor)
{
    std::shared_ptr<AstConstant> result;

    if (AstConstant *target_as_constant = dynamic_cast<AstConstant*>(target.get())) {
        // perform operations on these constants
        if (op_type == Operators::OP_negative) {
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
            CompilerError(
                LEVEL_ERROR,
                Msg_bitwise_operand_must_be_int,
                m_target->GetLocation(),
                type->GetName()
            )
        );
    } else if (m_op->GetType() & ARITHMETIC) {
        if (type != SymbolType::Builtin::ANY &&
            type != SymbolType::Builtin::INT &&
            type != SymbolType::Builtin::FLOAT &&
            type != SymbolType::Builtin::NUMBER) {
        
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_invalid_operator_for_type,
                m_target->GetLocation(),
                m_op->LookupStringValue(),
                type->GetName()
            )); 
        }

        visitor->Assert(type == SymbolType::Builtin::INT ||
            type == SymbolType::Builtin::FLOAT ||
            type == SymbolType::Builtin::NUMBER ||
            type == SymbolType::Builtin::ANY,
            CompilerError(
                LEVEL_ERROR,
                Msg_invalid_operator_for_type,
                m_target->GetLocation(),
                m_op->GetOperatorType(),
                type->GetName()
            )
        );
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

std::unique_ptr<Buildable> AstUnaryExpression::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    ASSERT(m_target != nullptr);

    chunk->Append(m_target->Build(visitor, mod));

    if (!m_folded) {
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (m_op->GetType() == ARITHMETIC) {
            uint8_t opcode = 0;

            if (m_op->GetOperatorType() == Operators::OP_negative) {
                opcode = NEG;
            }

            auto oper = BytecodeUtil::Make<RawOperation<>>();
            oper->opcode = opcode;
            oper->Accept<uint8_t>(rp);
            chunk->Append(std::move(oper));
        } else if (m_op->GetType() == LOGICAL) {
            if (m_op->GetOperatorType() == Operators::OP_logical_not) {
                // the label to jump to the very end, and set the result to false
                LabelId false_label = chunk->NewLabel();

                LabelId true_label = chunk->NewLabel();;

                { // compare lhs to 0 (false)
                    auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
                    instr_cmpz->opcode = CMPZ;
                    instr_cmpz->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_cmpz));
                }

                { // jump if they are not equal: i.e the value is true
                    auto instr_je = BytecodeUtil::Make<Jump>(JumpClass::JUMP_CLASS_JE, true_label);
                    chunk->Append(std::move(instr_je));
                }

                { // didn't skip past: load the false value
                    auto instr_load_false = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_false->opcode = LOAD_FALSE;
                    instr_load_false->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_false));
                }

                { // now, jump to the very end so we don't load the true value.
                    auto instr_jmp = BytecodeUtil::Make<Jump>(JumpClass::JUMP_CLASS_JMP, false_label);
                    chunk->Append(std::move(instr_jmp));
                }

                // skip to here to load true
                chunk->MarkLabel(true_label);

                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                
                { // here is where the value is true
                    auto instr_load_true = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_true->opcode = LOAD_TRUE;
                    instr_load_true->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_true));
                }

                // skip to here to avoid loading 'true' into the register
                chunk->MarkLabel(false_label);
            }
        }
    }

    return std::move(chunk);
}

void AstUnaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    m_target->Optimize(visitor, mod);

    if (m_op->GetOperatorType() == Operators::OP_positive) {
        m_folded = true;
    } else if (auto constant_value = ConstantFold(
        m_target,
        m_op->GetOperatorType(),
        visitor
    )) {
        m_target = constant_value;
        m_folded = true;
    }
}

void AstUnaryExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);
    if (!m_folded) {
        //ss << m_op->ToString();
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