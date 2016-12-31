#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstNull.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/ObjectType.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <iostream>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_type_specification(type_specification),
      m_assignment(assignment),
      m_assignment_already_visited(false)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    SymbolTypePtr_t symbol_type;

    if (m_type_specification == nullptr && m_assignment == nullptr) {
        // error; requires either type, or assignment.
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_missing_type_and_assignment, m_location, m_name));
    } else {
        // the type_strict flag means that errors will be shown if
        // the assignment type and the user-supplied type differ.
        // it is to be turned off for built-in (default) values
        // for example, the default type of Array is Any.
        // this flag will allow an Array(Float) to be constructed 
        // with an empty array of type Array(Any)
        bool type_strict = true;

        if (m_type_specification) {
            m_type_specification->Visit(visitor, mod);

            symbol_type = m_type_specification->GetSymbolType();

            // if no assignment provided, set the assignment to be the default value of the provided type
            if (m_assignment == nullptr && symbol_type != nullptr) {
                // Assign variable to the default value for the specified type.
                m_assignment = symbol_type->GetDefaultValue();
                // built-in assignment, turn off strict mode
                type_strict = false;
            }
        }

        if (m_assignment) {
            if (!m_assignment_already_visited) {
                // visit assignment
                m_assignment->Visit(visitor, mod);
            }

            // make sure type is compatible with assignment
            SymbolTypePtr_t assignment_type = m_assignment->GetSymbolType();
            ASSERT(assignment_type != nullptr);

            if (m_type_specification) {
                // symbol_type should be the user-specified type
                ASSERT(symbol_type != nullptr);


                if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                    // perform type promotion on incomplete generics.
                    // i.e: let x: Array = [1,2,3]
                    // will actually be of the type `Array(Int)`

                    if (assignment_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                        if (auto base = assignment_type->GetBaseType()) {
                            if (symbol_type->TypeEqual(*base)) {
                                // here is where type promotion is performed
                                symbol_type = assignment_type;
                            }
                        }
                    }
                }

                if (type_strict) {
                    visitor->Assert(symbol_type->TypeCompatible(*assignment_type, true),
                        CompilerError(Level_fatal, Msg_mismatched_types, m_assignment->GetLocation(),
                            symbol_type->GetName(), assignment_type->GetName()));
                }
            } else {
                // Set the type to be the deduced type from the expression.
                symbol_type = assignment_type;
            }
        }
    }

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier) {
        m_identifier->SetSymbolType(symbol_type);
        m_identifier->SetCurrentValue(m_assignment);
    }
}

void AstVariableDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_assignment != nullptr);

    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        m_assignment->Build(visitor, mod);

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // add instruction to store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    } else {
        // if assignment has side effects but variable is unused,
        // compile the assignment in anyway.
        if (m_assignment->MayHaveSideEffects()) {
            m_assignment->Build(visitor, mod);
        }
    }
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_assignment) {
        m_assignment->Optimize(visitor, mod);
    }
}

void AstVariableDeclaration::Recreate(std::ostringstream &ss)
{
    if (m_assignment) {
        ss << Keyword::ToString(Keyword_let) << " ";
        ss << m_name << "=";
        m_assignment->Recreate(ss);
    } else if (m_type_specification) {
        ss << m_name << ":";
        m_type_specification->Recreate(ss);
    } else {
        ss << m_name << "=??";
    }
}