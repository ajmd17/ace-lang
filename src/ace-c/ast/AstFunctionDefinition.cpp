#include <ace-c/ast/AstFunctionDefinition.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstFunctionDefinition::AstFunctionDefinition(const std::string &name,
    const std::shared_ptr<AstFunctionExpression> &expr,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_expr(expr)
{
}

void AstFunctionDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier) {
        // functions are implicitly const
        m_identifier->GetFlags() |= FLAG_CONST;
        m_identifier->SetSymbolType(m_expr->GetSymbolType());
        m_identifier->SetCurrentValue(m_expr);
    }
}

std::unique_ptr<Buildable> AstFunctionDefinition::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        // increment stack size before we build the expression
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

        // build function expression
        chunk->Append(m_expr->Build(visitor, mod));

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        
        // store on stack
        auto instr_push = BytecodeUtil::Make<RawOperation<>>();
        instr_push->opcode = PUSH;
        instr_push->Accept<uint8_t>(rp);
        chunk->Append(std::move(instr_push));
    }

    return std::move(chunk);
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
    m_expr->Optimize(visitor, mod);
}

void AstFunctionDefinition::Recreate(std::ostringstream &ss)
{
    ss << m_name << "=";
    m_expr->Recreate(ss);
}

Pointer<AstStatement> AstFunctionDefinition::Clone() const
{
    return CloneImpl();
}
