#include <ace-c/ast/AstFunctionCall.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <limits>
#include <iostream>

AstFunctionCall::AstFunctionCall(const std::string &name,
    const std::vector<std::shared_ptr<AstExpression>> &args,
    const SourceLocation &location)
    : AstIdentifier(name, location),
      m_args(args),
      m_return_type(SymbolType::Builtin::ANY),
      m_has_self_object(false)
{
}

void AstFunctionCall::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);

    // visit each argument
    for (auto &arg : m_args) {
		if (arg) {
			arg->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
		}
    }

    switch (m_properties.GetIdentifierType()) {
		case IDENTIFIER_TYPE_VARIABLE: {
			ASSERT(m_properties.GetIdentifier() != nullptr);
			m_properties.GetIdentifier()->IncUseCount();

			// NOTE: if we are in a function, and the variable we are loading is declared in a separate function,
			// we will show an error message saying that the variable must be passed as a parameter to be captured.
			// the reason for this is that any variables owned by the parent function will be immediately popped from the stack
			// when the parent function returns. That will mean the variables used here will reference garbage.
			// In the near feature, it'd be possible to automatically make a copy of those variables referenced and store them
			// on the stack of /this/ function.
			if (m_properties.IsInFunction() && (m_properties.GetIdentifier()->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
				// lookup the variable by depth to make sure it was declared in the current function
				// we do this to make sure it was declared in this scope.
				if (!mod->LookUpIdentifierDepth(m_name, m_properties.GetDepth())) {
					// add error that the variable must be passed as a parameter
					visitor->GetCompilationUnit()->GetErrorList().AddError(
						CompilerError(Level_fatal, Msg_closure_capture_must_be_parameter,
							m_location, m_name));
				}
			}

            // get the type of the referenced function we're calling
			SymbolTypePtr_t identifier_type = m_properties.GetIdentifier()->GetSymbolType();

            // continue if it is `Any` because we can't really assure that it is a function
			if (identifier_type != SymbolType::Builtin::ANY) {
                if (!(m_return_type = SemanticAnalyzer::SubstituteFunctionArgs(visitor, mod, identifier_type, m_args))) {
				    // not a function type
				    visitor->GetCompilationUnit()->GetErrorList().AddError(
					    CompilerError(Level_fatal, Msg_not_a_function, m_location, m_name));
                }
			}

			break;
		}
        case IDENTIFIER_TYPE_MODULE:
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_identifier_is_module, m_location, m_name));
            break;
        case IDENTIFIER_TYPE_TYPE:
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_identifier_is_type, m_location, m_name));
            break;
        case IDENTIFIER_TYPE_NOT_FOUND:
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_undeclared_identifier, m_location, m_name));
            break;
    }
}

void AstFunctionCall::BuildArgumentsStart(AstVisitor *visitor, Module *mod)
{
    uint8_t rp;

    // push a copy of each argument to the stack
    for (auto &arg : m_args) {
        ASSERT(arg != nullptr);

        arg->Build(visitor, visitor->GetCompilationUnit()->GetCurrentModule());

        // get active register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // now that it's loaded into the register, make a copy
        // add instruction to store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }

    // the reason we decrement the compiler's record of the stack size directly after
    // is because the function body will actually handle the management of the stack size,
    // so that the parameters are actually local variables to the function body.
    for (int i = 0; i < m_args.size(); i++) {
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void AstFunctionCall::BuildArgumentsEnd(AstVisitor *visitor, Module *mod)
{
    // pop arguments from stack
    Compiler::PopStack(visitor, m_args.size());
}

void AstFunctionCall::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_properties.GetIdentifier() != nullptr);

    BuildArgumentsStart(visitor, mod);

    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_properties.GetIdentifier()->GetStackLocation();
    int offset = stack_size - stack_location + (int)m_args.size();

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    if (!(m_properties.GetIdentifier()->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
        // load globally, rather than from offset.
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_INDEX, rp, (uint16_t)stack_location);
    } else {
        // load stack value at offset value into register
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)offset);
    }

    // invoke the function
    int argc = (int)m_args.size();
    if (m_has_self_object) {
        argc++;
    }

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)argc);

    BuildArgumentsEnd(visitor, mod);
}

void AstFunctionCall::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize each argument
    for (auto &arg : m_args) {
        if (arg) {
            arg->Optimize(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
        }
    }
}

void AstFunctionCall::Recreate(std::ostringstream &ss)
{
    ss << m_name << "(";
    for (size_t i = 0; i < m_args.size(); i++) {
        auto &arg = m_args[i];
        if (arg) {
            arg->Recreate(ss);
            if (i != m_args.size() - 1) {
                ss << ",";
            }
        }
    }
    ss << ")";
}

Pointer<AstStatement> AstFunctionCall::Clone() const
{
    return CloneImpl();
}

int AstFunctionCall::IsTrue() const
{
    // cannot deduce if return value is true
    return -1;
}

bool AstFunctionCall::MayHaveSideEffects() const
{
    // assume a function call has side effects
    // maybe we could detect this later
    return true;
}

SymbolTypePtr_t AstFunctionCall::GetSymbolType() const
{
    return m_return_type;
}
