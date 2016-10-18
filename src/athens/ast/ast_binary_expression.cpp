#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_constant.h>
#include <athens/ast/ast_integer.h>
#include <athens/ast_visitor.h>
#include <athens/operator.h>
#include <athens/emit/instruction.h>
#include <athens/emit/static_object.h>

#include <common/instructions.h>

#include <iostream>

/** Attemps to reduce a variable that is const literal to the actual value. */
static void OptimizeSide(std::shared_ptr<AstExpression> &side, AstVisitor *visitor)
{
    side->Optimize(visitor);

    AstVariable *side_as_var = nullptr;
    AstBinaryExpression *side_as_binop = nullptr;
    if ((side_as_var = dynamic_cast<AstVariable*>(side.get())) != nullptr) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (side_as_var->GetIdentifier() != nullptr) {
            if (side_as_var->GetIdentifier()->GetFlags() & (Flag_const)) {
                // the variable is a const, now we make sure that the current
                // value is a literal value
                auto value_sp = side_as_var->GetIdentifier()->GetCurrentValue().lock();
                AstConstant *constant_sp = dynamic_cast<AstConstant*>(value_sp.get());
                if (constant_sp != nullptr) {
                    // yay! we were able to retrieve the value that
                    // the variable is set to, so now we can use that
                    // at compile-time rather than using a variable.
                    side.reset(constant_sp);
                }
            }
        }
    } else if ((side_as_binop = dynamic_cast<AstBinaryExpression*>(side.get())) != nullptr) {
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

void AstBinaryExpression::Visit(AstVisitor *visitor)
{
    m_left->Visit(visitor);
    m_right->Visit(visitor);

    if (m_op->ModifiesValue()) {
        AstVariable *left_as_var = dynamic_cast<AstVariable*>(m_left.get());
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
    
    if (m_op->GetType() == ARITHMETIC) {
        uint8_t opcode;
        if (m_op == &Operator::operator_add) {
            opcode = ADD;
        } else if (m_op == &Operator::operator_subtract) {
            opcode = SUB;
        } else if (m_op == &Operator::operator_multiply) {
            opcode = MUL;
        }
        // TODO: handle more operators

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
                { // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstInteger(0, SourceLocation::eof));
                    auto constant_folded = ConstantFold(first, tmp, &Operator::operator_equals, visitor);
                    if (constant_folded != nullptr) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // value is equal to 0, therefore it is false.
                            // load the label address from static memory into register 0
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
                            // jump to end, the value is false
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t>(JMP, rp);
                        } else if (folded_value == 0) {
                            // do not jump at all, only accept the code that it is true
                        }
                    }
                }

                if (!folded) {
                    // load left-hand side into register 0
                    first->Build(visitor);

                    // increment register usage
                    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                    // since this is an AND operation, jump as soon as the lhs is determined to be false
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // load the value '0' (false) into register 1
                    visitor->GetCompilationUnit()->GetInstructionStream() << 
                        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
                    // compare lhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);
                    // unclaim the register
                    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                    // get current register index
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
                    // jump if they are equal: i.e the value is false
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JE, rp);
                }
            }

            // if we are at this point then lhs is true, so now test the rhs
            if (second != nullptr) {
                bool folded = false;
                { // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstInteger(0, SourceLocation::eof));
                    auto constant_folded = ConstantFold(second, tmp, &Operator::operator_equals, visitor);
                    if (constant_folded != nullptr) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // value is equal to 0, therefor it is false.
                            // load the label address from static memory into register 0
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
                            // jump to end, the value is false
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t>(JMP, rp);
                        } else if (folded_value == 0) {
                            // do not jump at all, only accept the code that it is true
                        }
                    }
                }

                if (!folded) {
                    // load right-hand side into register 1
                    second->Build(visitor);
                    // increment register usage
                    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                    // get register position
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    // load the value '0' (false) into register 1
                    visitor->GetCompilationUnit()->GetInstructionStream() << 
                        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
                    // compare rhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);

                    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                    // get current register index
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
                    // jump if they are equal: i.e the value is false
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JE, rp);
                }
                
            }

            // both values were true at this point so load the value '1' (for true)    
            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 1);

            // jump to the VERY end (so we don't load '0' value)
            // increment register usage
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get register position
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // load the label address from static memory into register 1
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);
            // jump
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(JMP, rp);

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
            // here is where the value is false
            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);

            // if true, skip to here to avoid loading '0' into the register
            true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
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
                { // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstInteger(0, SourceLocation::eof));
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
                            // jump to end, the value is true and we don't have to check the second half
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t>(JMP, rp);
                        }
                    }
                }

                if (!folded) {
                    // load left-hand side into register 0
                    first->Build(visitor);

                    // increment register usage
                    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                    // since this is an OR operation, jump as soon as the lhs is determined to be anything but 0
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    // load the value '0' (false) into register 1
                    visitor->GetCompilationUnit()->GetInstructionStream() << 
                        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
                    // compare lhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);
                    // unclaim the register
                    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                    // get current register index
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();


                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);
                    // jump if they are not equal: i.e the value is true
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JNE, rp);
                }
            }

            // if we are at this point then lhs is true, so now test the rhs
            if (second != nullptr) {
                bool folded = false;
                { // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstInteger(0, SourceLocation::eof));
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
                            visitor->GetCompilationUnit()->GetInstructionStream() <<
                                Instruction<uint8_t, uint8_t>(JMP, rp);
                        }
                    }
                }

                if (!folded) {
                    // load right-hand side into register 1
                    second->Build(visitor);
                    // increment register usage
                    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                    // get register position
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    // load the value '0' (false) into register 1
                    visitor->GetCompilationUnit()->GetInstructionStream() << 
                        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
                    // compare rhs to 0 (false)
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);

                    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                    // get current register index
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);
                    // jump if they are equal: i.e the value is true
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JNE, rp);
                }
                
            }

            // no values were true at this point so load the value '0' (for false)    
            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);

            // jump to the VERY end (so we don't load 'true' value)
            // increment register usage
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get register position
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // load the label address from static memory into register 1
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
            // jump if they are equal: i.e the value is false
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(JMP, rp);

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
            // here is where the value is true
            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 1);

            // skip to here to avoid loading '1' into the register
            false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);
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

                // load left-hand side into register 0
                m_left->Build(visitor);

                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

                // load right-hand side into register 1
                m_right->Build(visitor);

                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                visitor->GetCompilationUnit()->GetInstructionStream() << 
                    Instruction<uint8_t, uint8_t, uint8_t>(CMP, rp - 1, rp);

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                // load the label address from static memory into register 0
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, true_label.m_id);
                // jump if they are equal
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JE, rp);


                // values are not equal at this point
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
                // jump to the VERY end (so we don't load 'true' value)
                // increment register usage
                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
                // get register position
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                // load the label address from static memory into register 1
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, false_label.m_id);
                // jump if they are equal: i.e the value is false
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(JMP, rp);

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();


                true_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

                // values are equal
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 1);

                false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(true_label);
            } else {
                // load left-hand side into register
                m_left->Build(visitor);
            }
        }
    } else if (m_op->GetType() == ASSIGNMENT) {
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
        } else {
            // assignment/operation
            
            uint8_t opcode;
            if (m_op == &Operator::operator_add_assign) {
                opcode = ADD;
            } else if (m_op == &Operator::operator_subtract_assign) {
                opcode = SUB;
            } else if (m_op == &Operator::operator_multiply_assign) {
                opcode = MUL;
            }

            // load right-hand side into register 0
            m_right->Build(visitor);
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

            // load left-hand side into register 1
            m_left->Build(visitor);

            // perform operation
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);

            // now move the result into the left hand side
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
