#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_constant.h>
#include <athens/ast_visitor.h>
#include <athens/operator.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

#include <iostream>

/** Attemps to reduce a variable that is const literal to the actual value. */
static void OptimizeSide(std::shared_ptr<AstExpression> &side, AstVisitor *visitor)
{
    side->Optimize(visitor);

    AstVariable *side_as_var = nullptr;
    AstBinaryExpression *side_as_binop = nullptr;
    if (side_as_var = dynamic_cast<AstVariable*>(side.get())) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (side_as_var->GetIdentifier() != nullptr) {
            if (side_as_var->GetIdentifier()->GetFlags() & (Flag_const)) {
                // the variable is a const, now we make sure that the current
                // value is a literal value
                auto value_sp = side_as_var->GetIdentifier()->GetCurrentValue().lock();
                auto constant_sp = std::dynamic_pointer_cast<AstConstant>(value_sp);
                if (constant_sp != nullptr) {
                    // yay! we were able to retrieve the value that
                    // the variable is set to, so now we can use that
                    // at compile-time rather than using a variable.
                    side = constant_sp;
                }
            }
        }
    } else if (side_as_binop = dynamic_cast<AstBinaryExpression*>(side.get())) {
        if (side_as_binop->GetRight() == nullptr) {
            // right side has been optimized away, to just left side
            side = side_as_binop->GetLeft();
        }
    }
}

/** Attemps to evaluate the optimized expression at compile-time. */
static std::shared_ptr<AstConstant> ConstantFold(std::shared_ptr<AstExpression> &left, 
    std::shared_ptr<AstExpression> &right, const Operator *oper, AstVisitor *visitor)
{
    auto left_as_constant = std::dynamic_pointer_cast<AstConstant>(left);
    auto right_as_constant = std::dynamic_pointer_cast<AstConstant>(right);

    std::shared_ptr<AstConstant> result(nullptr);

    if (left_as_constant != nullptr && right_as_constant != nullptr) {
        // perform operations on these constants
        if (oper == &Operator::operator_add) {
            result = (*left_as_constant) + right_as_constant;
        } else if (oper == &Operator::operator_subtract) {
            result = (*left_as_constant) - right_as_constant;
        } else if (oper == &Operator::operator_multiply) {
            result = (*left_as_constant) * right_as_constant;
        } else if (oper == &Operator::operator_divide) {
            result = (*left_as_constant) / right_as_constant;
        } else if (oper == &Operator::operator_modulus) {
            result = (*left_as_constant) % right_as_constant;
        } else if (oper == &Operator::operator_bitwise_xor) {
            result = (*left_as_constant) ^ right_as_constant;
        } else if (oper == &Operator::operator_bitwise_and) {
            result = (*left_as_constant) & right_as_constant;
        } else if (oper == &Operator::operator_bitshift_left) {
            result = (*left_as_constant) << right_as_constant;
        } else if (oper == &Operator::operator_bitshift_right) {
            result = (*left_as_constant) >> right_as_constant;
        } else if (oper == &Operator::operator_logical_and) {
            result = (*left_as_constant) && right_as_constant;
        } else if (oper == &Operator::operator_logical_or) {
            result = (*left_as_constant) || right_as_constant;
        }
        // don't have to worry about assignment operations,
        // because at this point both sides are const and literal.
    }
    
    // one or both of the sides are not a constant
    return result;
}

AstBinaryExpression::AstBinaryExpression(const std::shared_ptr<AstExpression> &left,
        const std::shared_ptr<AstExpression> &right,
        const Operator *op,
        const SourceLocation &location)
    : AstExpression(location),
      m_left(left),
      m_right(right),
      m_op(op)
{
}

void AstBinaryExpression::Visit(AstVisitor *visitor)
{
    m_left->Visit(visitor);
    m_right->Visit(visitor);

    if (m_op->ModifiesValue()) {
        auto left_as_var = std::dynamic_pointer_cast<AstVariable>(m_left);
        if (left_as_var != nullptr) {
            // make sure we are not modifying a const
            if (left_as_var->GetIdentifier() != nullptr) {
                visitor->Assert(!(left_as_var->GetIdentifier()->GetFlags() & Flag_const),
                    CompilerError(Level_fatal, Msg_const_modified, 
                        m_left->GetLocation(), left_as_var->GetName()));
            }
        } else {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_cannot_modify_rvalue,
                    m_left->GetLocation()));
        }
    }
}

void AstBinaryExpression::Build(AstVisitor *visitor)
{
    AstBinaryExpression *left_as_binop = dynamic_cast<AstBinaryExpression*>(m_left.get());
    AstBinaryExpression *right_as_binop = dynamic_cast<AstBinaryExpression*>(m_right.get());
    
    if (m_op->ModifiesValue()) {
        if (m_op == &Operator::operator_assign) {
            // load right-hand side into register 0
            m_right->Build(visitor);
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

            // get current register index
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // get stack offset of left-hand side
            auto left_as_var = std::dynamic_pointer_cast<AstVariable>(m_left);
            if (left_as_var != nullptr) {
                int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
                int stack_location = left_as_var->GetIdentifier()->GetStackLocation();
                int offset = stack_size - stack_location;

                visitor->GetCompilationUnit()->GetInstructionStream() << 
                    Instruction<uint8_t, uint16_t, uint8_t>(MOV, offset, rp - 1);
            }

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        }
    } else {
        uint8_t opcode;
        if (m_op == &Operator::operator_add) {
            opcode = ADD;
        } else if (m_op == &Operator::operator_multiply) {
            opcode = MUL;
        }

        if (left_as_binop == nullptr && right_as_binop != nullptr) {
            // load right-hand side into register 0
            m_right->Build(visitor);
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

            // load left-hand side into register 1
            m_left->Build(visitor);

            // perform operation
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        } else {
            // load left-hand side into register 0
            m_left->Build(visitor);

            if (m_right != nullptr) {
                // right side has not been optimized away
                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                // load right-hand side into register 1
                m_right->Build(visitor);

                // perform operation
                uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                visitor->GetCompilationUnit()->GetInstructionStream() << 
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp - 1, rp, rp - 1);

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            }
        }
    }
}

void AstBinaryExpression::Optimize(AstVisitor *visitor)
{
    OptimizeSide(m_left, visitor);
    OptimizeSide(m_right, visitor);

    // check that we can further optimize the
    // binary expression by optimizing away the right
    // side, and combining the resulting value into
    // the left side of the operation.
    auto constant_value = ConstantFold(m_left, m_right, m_op, visitor);
    if (constant_value != nullptr) {
        // compile-time evaluation was successful
        m_left = constant_value;
        m_right = nullptr;
    }
}

int AstBinaryExpression::IsTrue() const
{
    if (m_right != nullptr) {
        // the right was not optimized away,
        // therefore we cannot determine whether or
        // not this expression would be true or false.
        return -1;
    }

    return m_left->IsTrue();
}
