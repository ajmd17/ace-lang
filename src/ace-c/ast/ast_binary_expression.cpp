#include <ace-c/ast/ast_binary_expression.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_function_call.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/ast/ast_integer.hpp>
#include <ace-c/ast/ast_true.hpp>
#include <ace-c/ast/ast_false.hpp>
#include <ace-c/ast/ast_member_access.hpp>
#include <ace-c/operator.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <cassert>

struct ExprInfo {
    AstExpression *left;
    AstBinaryExpression *left_as_binop;
    AstFunctionCall *left_as_call;

    AstExpression *right;
    AstBinaryExpression *right_as_binop;
    AstFunctionCall *right_as_call;
};

static void LoadLeftThenRight(AstVisitor *visitor, Module *mod, ExprInfo info)
{
    // load left-hand side into register 0
    info.left->Build(visitor, mod);

    // right side has not been optimized away
    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

    if (info.right != nullptr) {
        // load right-hand side into register 1
        info.right->Build(visitor, mod);
    }
}

/** Handles the right side before the left side. Used in the case that the
    right hand side is an expression, but the left hand side is just a value.
    If the left hand side is a function call, the right hand side will have to
    be temporarily stored on the stack.
*/
static void LoadRightThenLeft(AstVisitor *visitor, Module *mod, ExprInfo info)
{
    uint8_t rp;

    // load right-hand side into register 0
    info.right->Build(visitor, mod);
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    bool left_side_effects = info.left->MayHaveSideEffects();

    // if left is a function call, we have to move rhs to the stack!
    // otherwise, the function call will overwrite what's in register 0.
    int stack_size_before;
    if (left_side_effects) {
        // store value of the right hand side on the stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(PUSH, rp);
        stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    } else {
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    }

    // load left-hand side into register 1
    info.left->Build(visitor, mod);

    if (left_side_effects) {
        // now, we increase register usage to load rhs from the stack into register 1.
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        // get register position
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load from stack
        int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        int diff = stack_size_after - stack_size_before;
        assert(diff == 1);

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
        // pop from stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

/** Loads the left hand side and stores it on the stack.
    Then, the right hand side is loaded into a register,
    and the result is computed.
 */
static void LoadLeftAndStore(AstVisitor *visitor, Module *mod, ExprInfo info)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load left-hand side into register 0
    info.left->Build(visitor, mod);
    // get register position
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // store value of lhs on the stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(PUSH, rp);

    int stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    // increment stack size
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // do NOT increase register usage (yet)
    // load right-hand side into register 0, overwriting previous lhs
    info.right->Build(visitor, mod);

    // now, we increase register usage to load lhs from the stack into register 1.
    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    // get register position
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load from stack
    int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int diff = stack_size_after - stack_size_before;

    assert(diff == 1);

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
    // pop from stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
    // decrement stack size
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
}

/** Attemps to evaluate the optimized expression at compile-time. */
static std::shared_ptr<AstConstant> ConstantFold(std::shared_ptr<AstExpression> &left,
    std::shared_ptr<AstExpression> &right, const Operator *oper, AstVisitor *visitor)
{
    AstConstant *left_as_constant = dynamic_cast<AstConstant*>(left.get());
    AstConstant *right_as_constant = dynamic_cast<AstConstant*>(right.get());

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
        } else if (oper == &Operator::operator_equals) {
            result = left_as_constant->Equals(right_as_constant);
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

void AstBinaryExpression::Visit(AstVisitor *visitor, Module *mod)
{
    m_left->Visit(visitor, mod);
    m_right->Visit(visitor, mod);

    // make sure the types are compatible
    ObjectType left_type = m_left->GetObjectType();
    ObjectType right_type = m_right->GetObjectType();

    if (m_op->ModifiesValue()) {
        if (m_op->GetType() & BITWISE) {
            // no bitwise operators on floats allowed.
            // do not allow right-hand side to be 'Any', because it might change the data type.
            visitor->Assert((left_type  == ObjectType::type_builtin_int || left_type == ObjectType::type_builtin_any) &&
                            (right_type == ObjectType::type_builtin_int),
                CompilerError(Level_fatal, Msg_bitwise_operands_must_be_int, m_left->GetLocation(),
                    left_type.ToString(), right_type.ToString()));
        }

        if (!ObjectType::TypeCompatible(left_type, right_type, true)) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_mismatched_types,
                    m_left->GetLocation(), left_type.ToString(), right_type.ToString()));
        }

        AstVariable *left_as_var = nullptr;
        // check member access first
        AstMemberAccess *left_as_mem = dynamic_cast<AstMemberAccess*>(m_left.get());
        if (left_as_mem != nullptr) {
            AstIdentifier *last = left_as_mem->GetLast().get();
            left_as_var = dynamic_cast<AstVariable*>(last);
        } else {
            left_as_var = dynamic_cast<AstVariable*>(m_left.get());
        }

        if (left_as_var != nullptr) {
            if (left_as_var->GetIdentifier() != nullptr) {
                // make sure we are not modifying a const
                if (left_as_var->GetIdentifier()->GetFlags() & FLAG_CONST) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_const_modified,
                            m_left->GetLocation(), left_as_var->GetName()));
                }
            }
        } else {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_cannot_modify_rvalue,
                    m_left->GetLocation()));
        }
    } else {
        // allow 'Any' on right-hand side because we're not modifying value

        if (m_op->GetType() & BITWISE) {
            // no bitwise operators on floats allowed.
            if (!((left_type  == ObjectType::type_builtin_int || left_type == ObjectType::type_builtin_any) &&
                  (right_type == ObjectType::type_builtin_int || left_type == ObjectType::type_builtin_any))) {
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_bitwise_operands_must_be_int,
                        m_left->GetLocation(), left_type.ToString(), right_type.ToString()));
            }
        }

        // compare both sides because assignment does not matter in this case
        if (!(ObjectType::TypeCompatible(left_type,  right_type, false))) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_mismatched_types,
                    m_left->GetLocation(), left_type.ToString(), right_type.ToString()));
        }
    }
}

void AstBinaryExpression::Build(AstVisitor *visitor, Module *mod)
{
    AstBinaryExpression *left_as_binop = dynamic_cast<AstBinaryExpression*>(m_left.get());
    AstBinaryExpression *right_as_binop = dynamic_cast<AstBinaryExpression*>(m_right.get());
    AstFunctionCall *left_as_call = dynamic_cast<AstFunctionCall*>(m_left.get());
    AstFunctionCall *right_as_call = dynamic_cast<AstFunctionCall*>(m_right.get());

    ExprInfo info {
        m_left.get(), left_as_binop, left_as_call,
        m_right.get(), right_as_binop, right_as_call
    };

    if (m_op->GetType() == ARITHMETIC) {
        uint8_t opcode;
        if (m_op == &Operator::operator_add) {
            opcode = ADD;
        } else if (m_op == &Operator::operator_subtract) {
            opcode = SUB;
        } else if (m_op == &Operator::operator_multiply) {
            opcode = MUL;
        } else if (m_op == &Operator::operator_divide) {
            opcode = DIV;
        }

        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (left_as_binop == nullptr && right_as_binop != nullptr) {
            // if the right hand side is a binary operation,
            // we should build in the rhs first in order to
            // transverse the parse tree.
            LoadRightThenLeft(visitor, mod, info);
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);
        } else if (m_right != nullptr && m_right->MayHaveSideEffects()) {
            // lhs must be temporary stored on the stack,
            // to avoid the rhs overwriting it.
            if (m_left->MayHaveSideEffects()) {
                LoadLeftAndStore(visitor, mod, info);
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp - 1, rp, rp - 1);
            } else {
                // left  doesn't have side effects,
                // so just evaluate right without storing the lhs.
                LoadRightThenLeft(visitor, mod, info);
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);
            }
        } else {
            // normal usage, load left into register 0,
            // then load right into register 1.
            // rinse and repeat.
            LoadLeftThenRight(visitor, mod, info);
            if (m_right != nullptr) {
                // perform operation
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp - 1, rp, rp - 1);
            }
        }
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
    } else if (m_op->GetType() == BITWISE) {

    } else if (m_op->GetType() == LOGICAL) {
        std::shared_ptr<AstExpression> first = nullptr;
        std::shared_ptr<AstExpression> second = nullptr;

        if (left_as_binop == nullptr && right_as_binop != nullptr) {
            first = m_right;
            second = m_left;
        } else {
            first = m_left;
            second = m_right;
        }

        if (m_op == &Operator::operator_logical_and) {
            uint8_t rp;

            // the label to jump to the very end, and set the result to false
            StaticObject false_label;
            false_label.m_type = StaticObject::TYPE_LABEL;
            false_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

            StaticObject true_label;
            true_label.m_type = StaticObject::TYPE_LABEL;
            true_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            { // do first part of expression
                bool folded = false;
                // attempt to constant fold the values
                std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                auto constant_folded = ConstantFold(first, tmp, &Operator::operator_equals, visitor);
                if (constant_folded != nullptr) {
                    int folded_value = constant_folded->IsTrue();
                    folded = folded_value == 1 || folded_value == 0;

                    if (folded_value == 1) {
                        // value is equal to 0, therefore it is false.
                        // load the label address from static memory into register 0
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);

                        if (!ace::compiler::Config::use_static_objects) {
                            // fill with padding, for LOAD_ADDR instruction.
                            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                        }

                        // jump to end, the value is false
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t>(JMP, rp);
                    } else if (folded_value == 0) {
                        // do not jump at all, only accept the code that it is true
                    }
                }

                if (!folded) {
                    // load left-hand side into register 0
                    first->Build(visitor, mod);
                    // since this is an AND operation, jump as soon as the lhs is determined to be false
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // compare lhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(CMPZ, rp);

                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);

                    if (!ace::compiler::Config::use_static_objects) {
                        // fill with padding, for LOAD_ADDR instruction.
                        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                    }

                    // jump if they are equal: i.e the value is false
                    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(JE, rp);
                }
            }

            // if we are at this point then lhs is true, so now test the rhs
            if (second != nullptr) {
                bool folded = false;
                // attempt to constant fold the values
                std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                auto constant_folded = ConstantFold(second, tmp, &Operator::operator_equals, visitor);
                if (constant_folded != nullptr) {
                    int folded_value = constant_folded->IsTrue();
                    folded = folded_value == 1 || folded_value == 0;

                    if (folded_value == 1) {
                        // value is equal to 0, therefore it is false.
                        // load the label address from static memory into register 0
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);

                        if (!ace::compiler::Config::use_static_objects) {
                            // fill with padding, for LOAD_ADDR instruction.
                            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                        }
                        // jump to end, the value is false
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t>(JMP, rp);
                    } else if (folded_value == 0) {
                        // do not jump at all, only accept the code that it is true
                    }
                }

                if (!folded) {
                    // load right-hand side into register 1
                    second->Build(visitor, mod);
                    // get register position
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // compare rhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(CMPZ, rp);

                        // load the label address from static memory into register 0
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);

                    if (!ace::compiler::Config::use_static_objects) {
                        // fill with padding, for LOAD_ADDR instruction.
                        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                    }

                    // jump if they are equal: i.e the value is false
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JE, rp);
                }

            }

            // both values were true at this point so load the value 'true'
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);

            // jump to the VERY end (so we don't load 'false' value)
            // increment register usage
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get register position
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // load the label address from static memory into register 1
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

            if (!ace::compiler::Config::use_static_objects) {
                // fill with padding, for LOAD_ADDR instruction.
                visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
            }

            // jump
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(JMP, rp);

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
            // here is where the value is false
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);

            // if true, skip to here to avoid loading 'false' into the register
            true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);
        } else if (m_op == &Operator::operator_logical_or) {
            uint8_t rp;

            // the label to jump to the very end, and set the result to false
            StaticObject false_label;
            false_label.m_type = StaticObject::TYPE_LABEL;
            false_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

            StaticObject true_label;
            true_label.m_type = StaticObject::TYPE_LABEL;
            true_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            { // do first part of expression
                bool folded = false;
                // attempt to constant fold the values
                std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                auto constant_folded = ConstantFold(first, tmp, &Operator::operator_equals, visitor);
                if (constant_folded != nullptr) {
                    int folded_value = constant_folded->IsTrue();
                    folded = folded_value == 1 || folded_value == 0;

                    if (folded_value == 1) {
                        // do not jump at all, we still have to test the second half of the expression
                    } else if (folded_value == 0) {

                        // value is equal to 1
                            // load the label address from static memory into register 0
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

                        if (!ace::compiler::Config::use_static_objects) {
                            // fill with padding, for LOAD_ADDR instruction.
                            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                        }

                        // jump to end, the value is true and we don't have to check the second half
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t>(JMP, rp);
                    }
                }

                if (!folded) {
                    // load left-hand side into register 0
                    first->Build(visitor, mod);
                    // since this is an OR operation, jump as soon as the lhs is determined to be anything but 0
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
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
                        Instruction<uint8_t, uint8_t>(JNE, rp);
                }
            }

            // if we are at this point then lhs is true, so now test the rhs
            if (second != nullptr) {
                bool folded = false;
                { // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                    auto constant_folded = ConstantFold(second, tmp, &Operator::operator_equals, visitor);
                    if (constant_folded != nullptr) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // value is equal to 0
                        } else if (folded_value == 0) {
                                // value is equal to 1 so jump to end
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

                            if (!ace::compiler::Config::use_static_objects) {
                                // fill with padding, for LOAD_ADDR instruction.
                                visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                            }

                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t>(JMP, rp);
                        }
                    }
                }

                if (!folded) {
                    // load right-hand side into register 1
                    second->Build(visitor, mod);
                    // get register position
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // compare rhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(CMPZ, rp);

                        // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

                    if (!ace::compiler::Config::use_static_objects) {
                        // fill with padding, for LOAD_ADDR instruction.
                        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                    }

                    // jump if they are equal: i.e the value is true
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JNE, rp);
                }
            }

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
    } else if (m_op->GetType() == COMPARISON) {
        if (m_op == &Operator::operator_equals) {
            uint8_t rp;

            if (m_right != nullptr) {
                StaticObject true_label;
                true_label.m_type = StaticObject::TYPE_LABEL;
                true_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                StaticObject false_label;
                false_label.m_type = StaticObject::TYPE_LABEL;
                false_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();


                /*// load left-hand side into register 0
                m_left->Build(visitor, mod);

                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

                // load right-hand side into register 1
                m_right->Build(visitor, mod);

                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);*/

                if (left_as_binop == nullptr && right_as_binop != nullptr) {
                    // if the right hand side is a binary operation,
                    // we should build in the rhs first in order to
                    // transverse the parse tree.
                    LoadRightThenLeft(visitor, mod, info);
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp, rp - 1);
                } else if (m_right != nullptr && m_right->MayHaveSideEffects()) {
                    // lhs must be temporary stored on the stack,
                    // to avoid the rhs overwriting it.
                    if (m_left->MayHaveSideEffects()) {
                        LoadLeftAndStore(visitor, mod, info);
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);
                    } else {
                        // left  doesn't have side effects,
                        // so just evaluate right without storing the lhs.
                        LoadRightThenLeft(visitor, mod, info);
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp, rp - 1);
                    }
                } else {
                    // normal usage, load left into register 0,
                    // then load right into register 1.
                    // rinse and repeat.
                    LoadLeftThenRight(visitor, mod, info);
                    if (m_right != nullptr) {
                        // perform operation
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                        visitor->GetCompilationUnit()->GetInstructionStream() <<
                            Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);
                    }
                }

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                // load the label address from static memory into register 0
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);

                if (!ace::compiler::Config::use_static_objects) {
                    // fill with padding, for LOAD_ADDR instruction.
                    visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                }

                // jump if they are equal
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JE, rp);

                // values are not equal at this point
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

                // jump to the false label, the value is false at this point
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JMP, rp);

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);

                // values are equal
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);

                false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
            } else {
                // load left-hand side into register
                m_left->Build(visitor, mod);
            }
        }
    } else if (m_op->GetType() & ASSIGNMENT) {
        uint8_t rp;
        if (m_op == &Operator::operator_assign) {
            // load right-hand side into register 0
            m_right->Build(visitor, mod);
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        } else {
            // assignment/operation

            uint8_t opcode;
            if (m_op == &Operator::operator_add_assign) {
                opcode = ADD;
            } else if (m_op == &Operator::operator_subtract_assign) {
                opcode = SUB;
            } else if (m_op == &Operator::operator_multiply_assign) {
                opcode = MUL;
            } else if (m_op == &Operator::operator_divide_assign) {
                opcode = DIV;
            }

            // load right-hand side into register 0
            m_right->Build(visitor, mod);
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

            // load left-hand side into register 1
            m_left->Build(visitor, mod);

            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // perform operation
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);
        }

        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        AstVariable *left_as_var = dynamic_cast<AstVariable*>(m_left.get());
        if (left_as_var != nullptr && left_as_var->GetIdentifier() != nullptr) {
            // we are storing the rhs into the left,
            // so change access mode to store.
            left_as_var->SetAccessMode(ACCESS_MODE_STORE);
            left_as_var->Build(visitor, mod);
        } else {
            AstMemberAccess *left_as_mem = dynamic_cast<AstMemberAccess*>(m_left.get());
            if (left_as_mem != nullptr) {
                left_as_mem->SetAccessMode(ACCESS_MODE_STORE);
                left_as_mem->Build(visitor, mod);
            }
        }

        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
    }
}

void AstBinaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    Optimizer::OptimizeExpr(m_left, visitor, mod);
    Optimizer::OptimizeExpr(m_right, visitor, mod);

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

bool AstBinaryExpression::MayHaveSideEffects() const
{
    bool left_side_effects = m_left->MayHaveSideEffects();
    bool right_side_effects = false;

    if (m_right != nullptr) {
        right_side_effects = m_right->MayHaveSideEffects();
    }

    if (m_op->ModifiesValue()) {
        return true;
    }

    return left_side_effects || right_side_effects;
}

ObjectType AstBinaryExpression::GetObjectType() const
{
    ObjectType left_type = m_left->GetObjectType();
    ObjectType right_type = m_right->GetObjectType();

    if (m_op->ModifiesValue() &&
        (left_type.ToString() != ObjectType::type_builtin_any.ToString() &&
        right_type.ToString() == ObjectType::type_builtin_any.ToString())) {
        // special case for assignment operators.
        // cannot set a strict type to 'Any'.
       return ObjectType::type_builtin_undefined;
    }

    return ObjectType::FindCompatibleType(left_type, right_type);
}
