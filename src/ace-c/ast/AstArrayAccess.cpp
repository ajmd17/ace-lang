#include <ace-c/ast/AstArrayAccess.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/Module.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstArrayAccess::AstArrayAccess(const std::shared_ptr<AstExpression> &target,
    const std::shared_ptr<AstExpression> &index,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD | ACCESS_MODE_STORE),
      m_target(target),
      m_index(index)
{
}

void AstArrayAccess::Visit(AstVisitor *visitor, Module *mod)
{
    m_target->Visit(visitor, mod);
    m_index->Visit(visitor, mod);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();


    // check if target is an array
    if (target_type != SymbolType::Builtin::ANY) {
        if (!target_type->IsArrayType()) {
            // not an array type
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_not_an_array,
                m_location,
                target_type->GetName()
            ));
        }
    }
}

std::unique_ptr<Buildable> AstArrayAccess::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    ASSERT(m_index != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    bool target_side_effects = m_target->MayHaveSideEffects();
    bool index_side_effects = m_index->MayHaveSideEffects();
    
    uint8_t rp;
    uint8_t r0, r1;

    Compiler::ExprInfo info {
        m_target.get(), m_index.get()
    };

    if (!index_side_effects) {
        chunk->Append(Compiler::LoadLeftThenRight(visitor, mod, info));
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        r0 = rp - 1;
        r1 = rp;
    } else if (index_side_effects && !target_side_effects) {
        // load the index and store it
        chunk->Append(Compiler::LoadRightThenLeft(visitor, mod, info));
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        r0 = rp;
        r1 = rp - 1;
    } else {
        // load target, store it, then load the index
        chunk->Append(Compiler::LoadLeftAndStore(visitor, mod, info));
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        r0 = rp - 1;
        r1 = rp;
    }

    // do the operation
    if (m_access_mode == ACCESS_MODE_LOAD) {
        auto instr = BytecodeUtil::Make<RawOperation<>>();
        instr->opcode = LOAD_ARRAYIDX;
        instr->Accept<uint8_t>(r0); // destination
        instr->Accept<uint8_t>(r0); // source
        instr->Accept<uint8_t>(r1); // index

        chunk->Append(std::move(instr));
    }

    // unclaim register
    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

    return std::move(chunk);
}

void AstArrayAccess::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    ASSERT(m_index != nullptr);

    m_target->Optimize(visitor, mod);
    m_index->Optimize(visitor, mod);
}

void AstArrayAccess::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);
    ASSERT(m_index != nullptr);

    m_target->Recreate(ss);
    ss << "[";
    m_index->Recreate(ss);
    ss << "]";
}

Pointer<AstStatement> AstArrayAccess::Clone() const
{
    return CloneImpl();
}

int AstArrayAccess::IsTrue() const
{
    return -1;
}

bool AstArrayAccess::MayHaveSideEffects() const
{
    return m_target->MayHaveSideEffects() || m_index->MayHaveSideEffects() ||
        m_access_mode == ACCESS_MODE_STORE;
}

SymbolTypePtr_t AstArrayAccess::GetSymbolType() const
{
    ASSERT(m_target != nullptr);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    if (target_type->GetTypeClass() == TYPE_ARRAY) {
        SymbolTypePtr_t held_type = SymbolType::Builtin::UNDEFINED;

        if (target_type->GetGenericInstanceInfo().m_generic_args.size() == 1) {
            held_type = target_type->GetGenericInstanceInfo().m_generic_args[0].m_type;
            // todo: tuple?
        }

        ASSERT(held_type != nullptr);

        return held_type;
    }

    return SymbolType::Builtin::ANY;
}