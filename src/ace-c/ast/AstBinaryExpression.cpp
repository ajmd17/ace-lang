#include <ace-c/ast/AstBinaryExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstConstant.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>
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
    : AstExpression(location, ACCESS_MODE_LOAD),
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
            CompilerError(LEVEL_ERROR, Msg_bitwise_operands_must_be_int, m_location,
                left_type->GetName(), right_type->GetName()));
    }

    if (m_op->ModifiesValue()) {
        ASSERT(right_type != nullptr);

        if (!left_type->TypeCompatible(*right_type, true)) {
            CompilerError error(
                LEVEL_ERROR,
                Msg_mismatched_types,
                m_location,
                left_type->GetName(),
                right_type->GetName()
            );

            if (right_type == SymbolType::Builtin::ANY) {
                error = CompilerError(
                    LEVEL_ERROR,
                    Msg_implicit_any_mismatch,
                    m_location,
                    left_type->GetName()
                );
            }

            visitor->GetCompilationUnit()->GetErrorList().AddError(error);
        }

        /*AstVariable *left_as_var = nullptr;

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
        }*/

        /*if (left_as_var) {
            if (left_as_var->GetProperties().GetIdentifier()) {
                // make sure we are not modifying a const
                if (left_as_var->GetProperties().GetIdentifier()->GetFlags() & FLAG_CONST) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(LEVEL_ERROR, Msg_const_modified,
                            m_location, left_as_var->GetName()));
                }
            }
        }*/
        
        if (!(m_left->GetAccessOptions() & AccessMode::ACCESS_MODE_STORE)) {
            // cannot modify an rvalue
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_cannot_modify_rvalue,
                m_location
            ));
        }
    } else {
        // compare both sides because assignment does not matter in this case
        if (!left_type->TypeCompatible(*right_type, false)) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_mismatched_types,
                m_location,
                left_type->GetName(),
                right_type->GetName()
            ));
        }
    }
}

std::unique_ptr<Buildable> AstBinaryExpression::Build(AstVisitor *visitor, Module *mod)
{
    if (m_variable_declaration) {
        return m_variable_declaration->Build(visitor, mod);
    } else {
        std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

        Compiler::ExprInfo info {
            m_left.get(),
            m_right.get()
        };

        if (m_op->GetType() == ARITHMETIC || m_op->GetType() == BITWISE) {
            uint8_t opcode = 0;

            switch (m_op->GetOperatorType()) {
                case Operators::OP_add:
                    opcode = ADD;
                    break;
                case Operators::OP_subtract:
                    opcode = SUB;
                    break;
                case Operators::OP_multiply:
                    opcode = MUL;
                    break;
                case Operators::OP_divide:
                    opcode = DIV;
                    break;
                case Operators::OP_modulus:
                    opcode = MOD;
                    break;
                case Operators::OP_bitwise_and:
                    opcode = AND;
                    break;
                case Operators::OP_bitwise_or:
                    opcode = OR;
                    break;
                case Operators::OP_bitwise_xor:
                    opcode = XOR;
                    break;
                case Operators::OP_bitshift_left:
                    opcode = SHL;
                    break;
                case Operators::OP_bitshift_right:
                    opcode = SHR;
                    break;
            }

            chunk->Append(Compiler::BuildBinOp(opcode, visitor, mod, info));

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        } else if (m_op->GetType() == LOGICAL) {
            std::shared_ptr<AstExpression> first = nullptr;
            std::shared_ptr<AstExpression> second = nullptr;

            AstBinaryExpression *left_as_binop = dynamic_cast<AstBinaryExpression*>(m_left.get());
            AstBinaryExpression *right_as_binop = dynamic_cast<AstBinaryExpression*>(m_right.get());

            if (left_as_binop == nullptr && right_as_binop != nullptr) {
                first = m_right;
                second = m_left;
            } else {
                first = m_left;
                second = m_right;
            }

            if (m_op->GetOperatorType() == Operators::OP_logical_and) {
                uint8_t rp;

                LabelId false_label = chunk->NewLabel();
                LabelId true_label = chunk->NewLabel();

                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                // TODO: There is some duplicated code in here, needs to be cleaned up.

                { // do first part of expression
                    bool folded = false;
                    // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                    
                    if (auto constant_folded = Optimizer::ConstantFold(first, tmp, Operators::OP_equals, visitor)) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // value is equal to 0, therefore it is false.
                            // load the label address from static memory into register 0
                            auto instr_jmp = BytecodeUtil::Make<Jump>();
                            instr_jmp->opcode = JMP;
                            instr_jmp->label_id = false_label;
                            chunk->Append(std::move(instr_jmp));
                        } else if (folded_value == 0) {
                            // do not jump at all, only accept the code that it is true
                        }
                    }

                    if (!folded) {
                        // load left-hand side into register 0
                        chunk->Append(first->Build(visitor, mod));

                        // since this is an AND operation, jump as soon as the lhs is determined to be false
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        // compare lhs to 0 (false)
                        auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
                        instr_cmpz->opcode = CMPZ;
                        instr_cmpz->Accept<uint8_t>(rp);
                        chunk->Append(std::move(instr_cmpz));

                        // jump if they are equal: i.e the value is false
                        auto instr_je = BytecodeUtil::Make<Jump>();
                        instr_je->opcode = JE;
                        instr_je->label_id = false_label;
                        chunk->Append(std::move(instr_je));
                    }
                }

                // if we are at this point then lhs is true, so now test the rhs
                if (second != nullptr) {
                    bool folded = false;
                    // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                    
                    if (auto constant_folded = Optimizer::ConstantFold(second, tmp, Operators::OP_equals, visitor)) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // value is equal to 0, therefore it is false.
                            // load the label address from static memory into register 0
                            auto instr_jmp = BytecodeUtil::Make<Jump>();
                            instr_jmp->opcode = JMP;
                            instr_jmp->label_id = false_label;
                            chunk->Append(std::move(instr_jmp));
                        } else if (folded_value == 0) {
                            // do not jump at all, only accept the code that it is true
                        }
                    }

                    if (!folded) {
                        // load right-hand side into register 1
                        chunk->Append(second->Build(visitor, mod));

                        // get register position
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        // compare lhs to 0 (false)
                        auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
                        instr_cmpz->opcode = CMPZ;
                        instr_cmpz->Accept<uint8_t>(rp);
                        chunk->Append(std::move(instr_cmpz));

                        // jump if they are equal: i.e the value is false
                        auto instr_je = BytecodeUtil::Make<Jump>();
                        instr_je->opcode = JE;
                        instr_je->label_id = false_label;
                        chunk->Append(std::move(instr_je));
                    }
                }

                { // both values were true at this point so load the value 'true'
                    auto instr_load_true = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_true->opcode = LOAD_TRUE;
                    instr_load_true->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_true));
                }

                { // jump to the VERY end (so we don't load 'false' value)
                    auto instr_jmp = BytecodeUtil::Make<Jump>();
                    instr_jmp->opcode = JMP;
                    instr_jmp->label_id = false_label;
                    chunk->Append(std::move(instr_jmp));
                }

                chunk->MarkLabel(false_label);
                
                { // here is where the value is false
                    auto instr_load_false = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_false->opcode = LOAD_FALSE;
                    instr_load_false->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_false));
                }

                chunk->MarkLabel(true_label);
            } else if (m_op->GetOperatorType() == Operators::OP_logical_or) {
                uint8_t rp;

                LabelId false_label = chunk->NewLabel();
                LabelId true_label = chunk->NewLabel();

                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                { // do first part of expression
                    bool folded = false;
                    // attempt to constant fold the values
                    std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                    
                    if (auto constant_folded = Optimizer::ConstantFold(first, tmp, Operators::OP_equals, visitor)) {
                        int folded_value = constant_folded->IsTrue();
                        folded = folded_value == 1 || folded_value == 0;

                        if (folded_value == 1) {
                            // do not jump at all, we still have to test the second half of the expression
                        } else if (folded_value == 0) {
                            // jump to end, the value is true and we don't have to check the second half
                            auto instr_jmp = BytecodeUtil::Make<Jump>();
                            instr_jmp->opcode = JMP;
                            instr_jmp->label_id = true_label;
                            chunk->Append(std::move(instr_jmp));
                        }
                    }

                    if (!folded) {
                        // load left-hand side into register 0
                        chunk->Append(first->Build(visitor, mod));
                        // since this is an OR operation, jump as soon as the lhs is determined to be anything but 0
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        // compare lhs to 0 (false)
                        auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
                        instr_cmpz->opcode = CMPZ;
                        instr_cmpz->Accept<uint8_t>(rp);
                        chunk->Append(std::move(instr_cmpz));

                        auto instr_jne = BytecodeUtil::Make<Jump>();
                        instr_jne->opcode = JNE;
                        instr_jne->label_id = true_label;
                        chunk->Append(std::move(instr_jne));
                    }
                }

                // if we are at this point then lhs is true, so now test the rhs
                if (second != nullptr) {
                    bool folded = false;
                    { // attempt to constant fold the values
                        std::shared_ptr<AstExpression> tmp(new AstFalse(SourceLocation::eof));
                        
                        if (auto constant_folded = Optimizer::ConstantFold(second, tmp, Operators::OP_equals, visitor)) {
                            int folded_value = constant_folded->IsTrue();
                            folded = folded_value == 1 || folded_value == 0;

                            if (folded_value == 1) {
                                // value is equal to 0
                            } else if (folded_value == 0) {
                                // value is equal to 1 so jump to end
                                auto instr_jmp = BytecodeUtil::Make<Jump>();
                                instr_jmp->opcode = JMP;
                                instr_jmp->label_id = true_label;
                                chunk->Append(std::move(instr_jmp));
                            }
                        }
                    }

                    if (!folded) {
                        // load right-hand side into register 1
                        chunk->Append(second->Build(visitor, mod));
                        // get register position
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        // compare rhs to 0 (false)
                        auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
                        instr_cmpz->opcode = CMPZ;
                        instr_cmpz->Accept<uint8_t>(rp);
                        chunk->Append(std::move(instr_cmpz));

                        // jump if they are equal: i.e the value is true
                        auto instr_jne = BytecodeUtil::Make<Jump>();
                        instr_jne->opcode = JNE;
                        instr_jne->label_id = true_label;
                        chunk->Append(std::move(instr_jne));
                    }
                }

                { // no values were true at this point so load the value 'false'
                    auto instr_load_false = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_false->opcode = LOAD_FALSE;
                    instr_load_false->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_false));
                }

                { // jump to the VERY end (so we don't load 'true' value)
                    auto instr_jmp = BytecodeUtil::Make<Jump>();
                    instr_jmp->opcode = JMP;
                    instr_jmp->label_id = false_label;
                    chunk->Append(std::move(instr_jmp));
                }
                
                chunk->MarkLabel(true_label);

                { // here is where the value is true
                    auto instr_load_true = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_true->opcode = LOAD_TRUE;
                    instr_load_true->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_true));
                }

                chunk->MarkLabel(false_label);
            }
        } else if (m_op->GetType() == COMPARISON) {
            uint8_t rp;
            uint8_t opcode;
            
            bool swapped = false;

            switch (m_op->GetOperatorType()) {
                case Operators::OP_equals:
                    opcode = JNE;
                    break;
                case Operators::OP_not_eql:
                    opcode = JE;
                    break;
                case Operators::OP_less:
                    opcode = JGE;
                    break;
                case Operators::OP_less_eql:
                    opcode = JG;
                    break;
                case Operators::OP_greater:
                    opcode = JGE;
                    swapped = true;
                    break;
                case Operators::OP_greater_eql:
                    opcode = JG;
                    swapped = true;
                    break;
            }

            AstBinaryExpression *left_as_binop = dynamic_cast<AstBinaryExpression*>(m_left.get());
            AstBinaryExpression *right_as_binop = dynamic_cast<AstBinaryExpression*>(m_right.get());

            if (m_right != nullptr) {
                uint8_t r0, r1;

                LabelId true_label = chunk->NewLabel();
                LabelId false_label = chunk->NewLabel();

                if (left_as_binop == nullptr && right_as_binop != nullptr) {
                    // if the right hand side is a binary operation,
                    // we should build in the rhs first in order to
                    // transverse the parse tree.
                    chunk->Append(Compiler::LoadRightThenLeft(visitor, mod, info));
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    r0 = rp;
                    r1 = rp - 1;
                } else if (m_right != nullptr && m_right->MayHaveSideEffects()) {
                    // lhs must be temporary stored on the stack,
                    // to avoid the rhs overwriting it.
                    if (m_left->MayHaveSideEffects()) {
                        chunk->Append(Compiler::LoadLeftAndStore(visitor, mod, info));
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        r0 = rp - 1;
                        r1 = rp;
                    } else {
                        // left  doesn't have side effects,
                        // so just evaluate right without storing the lhs.
                        chunk->Append(Compiler::LoadRightThenLeft(visitor, mod, info));
                        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                        r0 = rp;
                        r1 = rp - 1;
                    }
                } else {
                    // normal usage, load left into register 0,
                    // then load right into register 1.
                    // rinse and repeat.
                    chunk->Append(Compiler::LoadLeftThenRight(visitor, mod, info));
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                    r0 = rp - 1;
                    r1 = rp;
                }

                if (swapped) {
                    std::swap(r0, r1);
                }

                { // perform operation
                    auto instr_cmp = BytecodeUtil::Make<RawOperation<>>();
                    instr_cmp->opcode = CMP;
                    instr_cmp->Accept<uint8_t>(r0);
                    instr_cmp->Accept<uint8_t>(r1);
                    chunk->Append(std::move(instr_cmp));
                }

                visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                { // jump if they are equal
                    auto instr_jump = BytecodeUtil::Make<Jump>();
                    instr_jump->opcode = opcode;
                    instr_jump->label_id = true_label;
                    chunk->Append(std::move(instr_jump));
                }
                
                { // values are not equal at this point
                    auto instr_load_true = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_true->opcode = LOAD_TRUE;
                    instr_load_true->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_true));
                }

                { // jump to the false label, the value is false at this point
                    auto instr_jump = BytecodeUtil::Make<Jump>();
                    instr_jump->opcode = JMP;
                    instr_jump->label_id = false_label;
                    chunk->Append(std::move(instr_jump));
                }
                
                chunk->MarkLabel(true_label);

                { // values are equal
                    auto instr_load_false = BytecodeUtil::Make<RawOperation<>>();
                    instr_load_false->opcode = LOAD_FALSE;
                    instr_load_false->Accept<uint8_t>(rp);
                    chunk->Append(std::move(instr_load_false));
                }

                chunk->MarkLabel(false_label);
            } else {
                // load left-hand side into register
                // right-hand side has been optimized away
                chunk->Append(m_left->Build(visitor, mod));
            }
        } else if (m_op->GetType() & ASSIGNMENT) {
            uint8_t rp;

            if (m_op->GetOperatorType() == Operators::OP_assign) {
                // load right-hand side into register 0
                chunk->Append(m_right->Build(visitor, mod));
                visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            } else {
                // assignment/operation
                uint8_t opcode = 0;

                switch (m_op->GetOperatorType()) {
                    case Operators::OP_add_assign:
                        opcode = ADD;
                        break;
                    case Operators::OP_subtract_assign:
                        opcode = SUB;
                        break;
                    case Operators::OP_multiply_assign:
                        opcode = MUL;
                        break;
                    case Operators::OP_divide_assign:
                        opcode = DIV;
                        break;
                    case Operators::OP_modulus_assign:
                        opcode = MOD;
                        break;
                }

                chunk->Append(Compiler::BuildBinOp(opcode, visitor, mod, info));
            }

            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            if (m_left->GetAccessOptions() & AccessMode::ACCESS_MODE_STORE) {
                m_left->SetAccessMode(AccessMode::ACCESS_MODE_STORE);
                chunk->Append(m_left->Build(visitor, mod));
            }

            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        }

        return std::move(chunk);
    }
}

void AstBinaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_variable_declaration) {
        m_variable_declaration->Optimize(visitor, mod);
    } else {
        Optimizer::OptimizeExpr(m_left, visitor, mod);
        Optimizer::OptimizeExpr(m_right, visitor, mod);

        // check that we can further optimize the
        // binary expression by optimizing away the right
        // side, and combining the resulting value into
        // the left side of the operation.
        if (auto constant_value = Optimizer::ConstantFold(
            m_left,
            m_right,
            m_op->GetOperatorType(),
            visitor
        )) {
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
        //ss << m_op->ToString();
        m_right->Recreate(ss);
    }
}

Pointer<AstStatement> AstBinaryExpression::Clone() const
{
    return CloneImpl();
}

int AstBinaryExpression::IsTrue() const
{
    // if (m_member_access) {
    //     return m_member_access->IsTrue();
    // } else {
        if (m_right) {
            // the right was not optimized away,
            // therefore we cannot determine whether or
            // not this expression would be true or false.
            return -1;
        }

        return m_left->IsTrue();
    //}
}

bool AstBinaryExpression::MayHaveSideEffects() const
{
    // if (m_member_access) {
    //     return m_member_access->MayHaveSideEffects();
    // } else {
        bool left_side_effects = m_left->MayHaveSideEffects();
        bool right_side_effects = false;

        if (m_right) {
            right_side_effects = m_right->MayHaveSideEffects();
        }

        if (m_op->ModifiesValue()) {
            return true;
        }

        return left_side_effects || right_side_effects;
    //}
}

SymbolTypePtr_t AstBinaryExpression::GetSymbolType() const
{
    // if (m_member_access) {
    //     return m_member_access->GetSymbolType();
    // } else {
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
    //}
}

std::shared_ptr<AstVariableDeclaration> AstBinaryExpression::CheckLazyDeclaration(AstVisitor *visitor, Module *mod)
{
    if (ace::compiler::Config::lazy_declarations && (m_op->GetOperatorType() == Operators::OP_assign)) {
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
