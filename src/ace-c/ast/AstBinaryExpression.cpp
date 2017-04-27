#include <ace-c/ast/AstBinaryExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstFunctionCall.hpp>
#include <ace-c/ast/AstConstant.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>
#include <ace-c/ast/AstMemberAccess.hpp>
#include <ace-c/ast/AstArrayAccess.hpp>
#include <ace-c/Operator.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>

#include <iostream>

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
    ASSERT(m_left != nullptr);
    ASSERT(m_right != nullptr);

    // check for lazy declaration first
    if ((m_variable_declaration = CheckLazyDeclaration(visitor, mod))) {
        m_variable_declaration->Visit(visitor, mod);
        // return, our work here is done
        return;
    }

    m_left->Visit(visitor, mod);
    m_right->Visit(visitor, mod);

    SymbolTypePtr_t left_type  = m_left->GetSymbolType();
    SymbolTypePtr_t right_type = m_right->GetSymbolType();

    if (m_op->GetType() & BITWISE) {
        // no bitwise operators on floats allowed.
        visitor->Assert((left_type == SymbolType::Builtin::INT || left_type == SymbolType::Builtin::ANY) &&
            (right_type == SymbolType::Builtin::INT || right_type == SymbolType::Builtin::ANY),
            CompilerError(Level_fatal, Msg_bitwise_operands_must_be_int, m_location,
                left_type->GetName(), right_type->GetName()));
    }

    if (m_op->ModifiesValue()) {
        ASSERT(right_type != nullptr);

        if (!left_type->TypeCompatible(*right_type, true)) {
            CompilerError error(Level_fatal, Msg_mismatched_types,
                m_location, left_type->GetName(), right_type->GetName());

            if (right_type == SymbolType::Builtin::ANY) {
                error = CompilerError(Level_fatal, Msg_implicit_any_mismatch,
                    m_location, left_type->GetName());
            }

            visitor->GetCompilationUnit()->GetErrorList().AddError(error);
        }

        AstVariable *left_as_var = nullptr;

        if (auto *left_as_mem = dynamic_cast<AstMemberAccess*>(m_left.get())) {
            AstIdentifier *last = left_as_mem->GetLast().get();
            left_as_var = dynamic_cast<AstVariable*>(last);
        } else if (auto *left_as_mod = dynamic_cast<AstModuleAccess*>(m_left.get())) {
            AstModuleAccess *target = left_as_mod;
            // loop until null or found
            while (left_as_var == nullptr && target != nullptr) {
                if (!(left_as_var = dynamic_cast<AstVariable*>(target->GetExpression().get()))) {
                    // check if rhs of module access is also a module access
                    target = dynamic_cast<AstModuleAccess*>(target->GetExpression().get());
                }
            }
        } else {
            left_as_var = dynamic_cast<AstVariable*>(m_left.get());
        }

        if (left_as_var) {
            if (left_as_var->GetProperties().GetIdentifier()) {
                // make sure we are not modifying a const
                if (left_as_var->GetProperties().GetIdentifier()->GetFlags() & FLAG_CONST) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_const_modified,
                            m_location, left_as_var->GetName()));
                }
            }
        } else {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_cannot_modify_rvalue,
                    m_location));
        }
    } else {
        // compare both sides because assignment does not matter in this case
        if (!left_type->TypeCompatible(*right_type, false)) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_mismatched_types,
                    m_location, left_type->GetName(), right_type->GetName()));
        }
    }
}

void AstBinaryExpression::Build(AstVisitor *visitor, Module *mod)
{
    if (m_member_access) {
        m_member_access->Build(visitor, mod);
    } else if (m_variable_declaration) {
        m_variable_declaration->Build(visitor, mod);
    } else {
        AstBinaryExpression *left_as_binop = dynamic_cast<AstBinaryExpression*>(m_left.get());
        AstBinaryExpression *right_as_binop = dynamic_cast<AstBinaryExpression*>(m_right.get());
        
        Compiler::ExprInfo info {
            m_left.get(), m_right.get()
        };

        if (m_op->GetType() == ARITHMETIC) {
            uint8_t opcode = 0;

            if (m_op == &Operator::operator_add) {
                opcode = ADD;
            } else if (m_op == &Operator::operator_subtract) {
                opcode = SUB;
            } else if (m_op == &Operator::operator_multiply) {
                opcode = MUL;
            } else if (m_op == &Operator::operator_divide) {
                opcode = DIV;
            }

            uint8_t rp;

            if (left_as_binop == nullptr && right_as_binop != nullptr) {
                // if the right hand side is a binary operation,
                // we should build in the rhs first in order to
                // transverse the parse tree.
                Compiler::LoadRightThenLeft(visitor, mod, info);
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);
            } else if (m_right != nullptr && m_right->MayHaveSideEffects()) {
                // lhs must be temporary stored on the stack,
                // to avoid the rhs overwriting it.
                if (m_left->MayHaveSideEffects()) {
                    Compiler::LoadLeftAndStore(visitor, mod, info);
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp - 1, rp, rp - 1);
                } else {
                    // left  doesn't have side effects,
                    // so just evaluate right without storing the lhs.
                    Compiler::LoadRightThenLeft(visitor, mod, info);
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(opcode, rp, rp - 1, rp - 1);
                }
            } else {
                Compiler::LoadLeftThenRight(visitor, mod, info);
                if (m_right) {
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
                    
                    if (auto constant_folded = Optimizer::ConstantFold(first, tmp, &Operator::operator_equals, visitor)) {
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
                        visitor->GetCompilationUnit()->GetInstructionStream() << 
                            Instruction<uint8_t, uint8_t>(JE, rp);
                    }
                }

                // if we are at this point then lhs is true, so now test the rhs
                if (second) {
                    bool folded = false;
                    // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                    
                    if (auto constant_folded = Optimizer::ConstantFold(second, tmp, &Operator::operator_equals, visitor)) {
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
                    
                    if (auto constant_folded = Optimizer::ConstantFold(first, tmp, &Operator::operator_equals, visitor)) {
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
                if (second) {
                    bool folded = false;
                    { // attempt to constant fold the values
                        std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                        
                        if (auto constant_folded = Optimizer::ConstantFold(second, tmp, &Operator::operator_equals, visitor)) {
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
            uint8_t rp;
            uint8_t opcode;
            
            bool swapped = false;

            if (m_op == &Operator::operator_equals) {
                opcode = JNE;
            } else if (m_op == &Operator::operator_not_eql) {
                opcode = JE;
            } else if (m_op == &Operator::operator_less) {
                opcode = JGE;
            } else if (m_op == &Operator::operator_less_eql) {
                opcode = JG;
            } else if (m_op == &Operator::operator_greater) {
                opcode = JGE;
                swapped = true;
            } else if (m_op == &Operator::operator_greater_eql) {
                opcode = JG;
                swapped = true;
            }

            if (m_right) {
                uint8_t r0, r1;

                StaticObject true_label;
                true_label.m_type = StaticObject::TYPE_LABEL;
                true_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                StaticObject false_label;
                false_label.m_type = StaticObject::TYPE_LABEL;
                false_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                if (left_as_binop == nullptr && right_as_binop != nullptr) {
                    // if the right hand side is a binary operation,
                    // we should build in the rhs first in order to
                    // transverse the parse tree.
                    Compiler::LoadRightThenLeft(visitor, mod, info);
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    r0 = rp;
                    r1 = rp - 1;
                } else if (m_right != nullptr && m_right->MayHaveSideEffects()) {
                    // lhs must be temporary stored on the stack,
                    // to avoid the rhs overwriting it.
                    if (m_left->MayHaveSideEffects()) {
                        Compiler::LoadLeftAndStore(visitor, mod, info);
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        r0 = rp - 1;
                        r1 = rp;
                    } else {
                        // left  doesn't have side effects,
                        // so just evaluate right without storing the lhs.
                        Compiler::LoadRightThenLeft(visitor, mod, info);
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        r0 = rp;
                        r1 = rp - 1;
                    }
                } else {
                    // normal usage, load left into register 0,
                    // then load right into register 1.
                    // rinse and repeat.
                    Compiler::LoadLeftThenRight(visitor, mod, info);
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    r0 = rp - 1;
                    r1 = rp;
                }

                if (swapped) {
                    std::swap(r0, r1);
                }

                // perform operation
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t>(CMP, r0, r1);

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
                    Instruction<uint8_t, uint8_t>(opcode, rp);

                // values are not equal at this point
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);

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
                    Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);

                false_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(false_label);
            } else {
                // load left-hand side into register
                // right-hand side has been optimized away
                m_left->Build(visitor, mod);
            }
        } else if (m_op->GetType() & ASSIGNMENT) {
            uint8_t rp;
            if (m_op == &Operator::operator_assign) {
                // load right-hand side into register 0
                m_right->Build(visitor, mod);
                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            } else {
                // assignment/operation
                uint8_t opcode = 0;
                
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

            AstVariable *left_as_var = nullptr;

            if ((left_as_var = dynamic_cast<AstVariable*>(m_left.get())) && left_as_var->GetProperties().GetIdentifier()) {
                // we are storing the rhs into the left,
                // so change access mode to store.
                left_as_var->GetProperties().SetAccessMode(ACCESS_MODE_STORE);
                left_as_var->Build(visitor, mod);
            } else if (AstModuleAccess *left_as_mod = dynamic_cast<AstModuleAccess*>(m_left.get())) {
                AstModuleAccess *target = left_as_mod;
                // loop until null or found
                while (left_as_var == nullptr && target != nullptr) {
                    if (!(left_as_var = dynamic_cast<AstVariable*>(target->GetExpression().get()))) {
                        // check if rhs of module access is also a module access
                        target = dynamic_cast<AstModuleAccess*>(target->GetExpression().get());
                    }
                }

                if (left_as_var) {
                    left_as_var->GetProperties().SetAccessMode(ACCESS_MODE_STORE);
                    left_as_var->Build(visitor, mod);
                }
            } else if (auto *left_as_mem = dynamic_cast<AstMemberAccess*>(m_left.get())) {
                left_as_mem->SetAccessMode(ACCESS_MODE_STORE);
                left_as_mem->Build(visitor, mod);
            } else if (auto *left_as_array = dynamic_cast<AstArrayAccess*>(m_left.get())) {
                left_as_array->SetAccessMode(ACCESS_MODE_STORE);
                left_as_array->Build(visitor, mod);
            }

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        }
    }
}

void AstBinaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_member_access) {
        m_member_access->Optimize(visitor, mod);
    } else if (m_variable_declaration) {
        m_variable_declaration->Optimize(visitor, mod);
    } else {
        Optimizer::OptimizeExpr(m_left, visitor, mod);
        Optimizer::OptimizeExpr(m_right, visitor, mod);

        // check that we can further optimize the
        // binary expression by optimizing away the right
        // side, and combining the resulting value into
        // the left side of the operation.
        if (auto constant_value = Optimizer::ConstantFold(m_left, m_right, m_op, visitor)) {
            // compile-time evaluation was successful
            m_left = constant_value;
            m_right = nullptr;
        }
    }
}

void AstBinaryExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_left != nullptr);
    m_left->Recreate(ss);

    if (m_right) {
        ss << m_op->ToString();
        m_right->Recreate(ss);
    }
}

Pointer<AstStatement> AstBinaryExpression::Clone() const
{
    return CloneImpl();
}

int AstBinaryExpression::IsTrue() const
{
    if (m_member_access) {
        return m_member_access->IsTrue();
    } else {
        if (m_right) {
            // the right was not optimized away,
            // therefore we cannot determine whether or
            // not this expression would be true or false.
            return -1;
        }

        return m_left->IsTrue();
    }
}

bool AstBinaryExpression::MayHaveSideEffects() const
{
    if (m_member_access) {
        return m_member_access->MayHaveSideEffects();
    } else {
        bool left_side_effects = m_left->MayHaveSideEffects();
        bool right_side_effects = false;

        if (m_right) {
            right_side_effects = m_right->MayHaveSideEffects();
        }

        if (m_op->ModifiesValue()) {
            return true;
        }

        return left_side_effects || right_side_effects;
    }
}

SymbolTypePtr_t AstBinaryExpression::GetSymbolType() const
{
    if (m_member_access) {
        return m_member_access->GetSymbolType();
    } else {
        ASSERT(m_left != nullptr);

        SymbolTypePtr_t l_type_ptr = m_left->GetSymbolType();
        ASSERT(l_type_ptr != nullptr);

        if (m_right) {
            // the right was not optimized away,
            // return type promotion
            SymbolTypePtr_t r_type_ptr = m_right->GetSymbolType();

            ASSERT(r_type_ptr != nullptr);

            return SymbolType::TypePromotion(l_type_ptr, r_type_ptr, true);
        } else {
            // right was optimized away, return only left type
            return l_type_ptr;
        }
    }
}

std::shared_ptr<AstVariableDeclaration> AstBinaryExpression::CheckLazyDeclaration(AstVisitor *visitor, Module *mod)
{
    if (ace::compiler::Config::lazy_declarations && m_op == &Operator::operator_assign) {
        if (AstVariable *left_as_var = dynamic_cast<AstVariable*>(m_left.get())) {
            std::string var_name = left_as_var->GetName();
            // lookup variable name
            if (mod->LookUpIdentifier(var_name, false)) {
                return nullptr;
            }
            // not found as variable name
            // look up in the global module
            if (visitor->GetCompilationUnit()->GetGlobalModule()->LookUpIdentifier(var_name, false)) {
                return nullptr;
            }

            // check all modules for one with the same name
            if (visitor->GetCompilationUnit()->LookupModule(var_name)) {
                return nullptr;
            }

            return std::shared_ptr<AstVariableDeclaration>(
                new AstVariableDeclaration(var_name,
                    nullptr, m_right, m_left->GetLocation()));
        }
    }

    return nullptr;
}
