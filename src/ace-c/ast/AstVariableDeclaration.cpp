#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <iostream>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstExpression> &assignment,
    bool is_const,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_type_specification(type_specification),
      m_assignment(assignment),
      m_is_const(is_const),
      m_assignment_already_visited(false)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    SymbolTypePtr_t symbol_type;

    if (m_type_specification == nullptr && m_assignment == nullptr) {
        // error; requires either type, or assignment.
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_missing_type_and_assignment,
            m_location,
            m_name
        ));
    } else {
        if (m_assignment != nullptr) {
            m_real_assignment = m_assignment;
        }

        // the type_strict flag means that errors will be shown if
        // the assignment type and the user-supplied type differ.
        // it is to be turned off for built-in (default) values
        // for example, the default type of Array is Any.
        // this flag will allow an Array(Float) to be constructed 
        // with an empty array of type Array(Any)
        bool is_type_strict = true;

        if (m_type_specification != nullptr) {
            m_type_specification->Visit(visitor, mod);

            symbol_type = m_type_specification->GetSymbolType();
            ASSERT(symbol_type != nullptr);

            if (symbol_type == BuiltinTypes::ANY) {
                // Any type is reserved for method parameters
                visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                    LEVEL_ERROR,
                    Msg_any_reserved_for_parameters,
                    m_location
                ));
            }

            // if no assignment provided, set the assignment to be the default value of the provided type
            if (m_real_assignment == nullptr) {
                if (symbol_type->GetDefaultValue() != nullptr) {
                    // Assign variable to the default value for the specified type.
                    m_real_assignment = symbol_type->GetDefaultValue();
                    // built-in assignment, turn off strict mode
                    is_type_strict = false;
                } else {
                    // no default assignment for this type
                    visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                        LEVEL_ERROR,
                        Msg_type_no_default_assignment,
                        m_location,
                        symbol_type->GetName()
                    ));
                }
            }

            
        }

        if (m_real_assignment != nullptr) {
            if (!m_assignment_already_visited) {
                // visit assignment
                m_real_assignment->Visit(visitor, mod);
            }

            if (m_assignment != nullptr) { // has received an explicit assignment
                // make sure type is compatible with assignment
                SymbolTypePtr_t assignment_type = m_real_assignment->GetSymbolType();
                ASSERT(assignment_type != nullptr);

                if (m_type_specification != nullptr) {
                    // symbol_type should be the user-specified type
                    ASSERT(symbol_type != nullptr);

                    bool is_left_incomplete = false;

                    switch (symbol_type->GetTypeClass()) {
                        case TYPE_GENERIC:
                            is_left_incomplete = true;
                            break;
                    }


                    if (is_left_incomplete) {
                        // perform type promotion on incomplete generics.
                        // i.e: let x: Array = [1,2,3]
                        // will actually be of the type `Array(Int)`

                        // NOTE: removed because if somebody writes a: Array = [1,2,3]
                        // and later wants to assign it to ["hi"] they shouldn't receive an error,
                        // as they did not explicitly specify that it is Array<Int> in this case.

                        // Added back in

                        if (assignment_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                            if (auto base = assignment_type->GetBaseType()) {
                                if (symbol_type->TypeEqual(*base)) {
                                    // here is where type promotion is performed
                                    
                                    symbol_type = assignment_type;
                                }
                            }
                        }
                    }

                    if (is_type_strict) {
                        SymbolTypePtr_t comparison_type = symbol_type;

                        // unboxing values
                        // note that this is below the default assignment check,
                        // because these "box" types may have a default assignment of their own
                        // (or they may intentionally not have one)
                        // e.g Maybe(T) defaults to null, and Const(T) has no assignment.
                        if (symbol_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                            if (symbol_type->IsBoxedType()) {
                                comparison_type = symbol_type->GetGenericInstanceInfo().m_generic_args[0].m_type;
                                ASSERT(comparison_type != nullptr);
                            }
                        }

                        if (!comparison_type->TypeCompatible(*assignment_type, true)) {
                            CompilerError error(
                                LEVEL_ERROR,
                                Msg_mismatched_types,
                                m_real_assignment->GetLocation(),
                                comparison_type->GetName(),
                                assignment_type->GetName()
                            );

                            if (assignment_type == BuiltinTypes::ANY) {
                                error = CompilerError(
                                    LEVEL_ERROR,
                                    Msg_implicit_any_mismatch,
                                    m_real_assignment->GetLocation(),
                                    comparison_type->GetName()
                                );
                            }

                            visitor->GetCompilationUnit()->GetErrorList().AddError(error);
                        }
                    }
                } else {
                    // Set the type to be the deduced type from the expression.
                    symbol_type = assignment_type;
                }
            }
        }
    }

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier != nullptr) {
        if (m_is_const) {
            m_identifier->SetFlags(m_identifier->GetFlags() | IdentifierFlags::FLAG_CONST);
        }

        m_identifier->SetSymbolType(symbol_type);
        m_identifier->SetCurrentValue(m_real_assignment);
    }
}

std::unique_ptr<Buildable> AstVariableDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    ASSERT(m_real_assignment != nullptr);

    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        const int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        chunk->Append(m_real_assignment->Build(visitor, mod));

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        { // add instruction to store on stack
            auto instr_push = BytecodeUtil::Make<RawOperation<>>();
            instr_push->opcode = PUSH;
            instr_push->Accept<uint8_t>(rp);
            chunk->Append(std::move(instr_push));
        }

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    } else {
        // if assignment has side effects but variable is unused,
        // compile the assignment in anyway.
        if (m_real_assignment->MayHaveSideEffects()) {
            chunk->Append(m_real_assignment->Build(visitor, mod));
        }
    }

    return std::move(chunk);
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_real_assignment != nullptr) {
        m_real_assignment->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstVariableDeclaration::Clone() const
{
    return CloneImpl();
}
