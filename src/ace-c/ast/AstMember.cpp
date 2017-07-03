#include <ace-c/ast/AstMember.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/hasher.hpp>

#include <iostream>

AstMember::AstMember(
    const std::string &field_name,
    const std::shared_ptr<AstExpression> &target,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD | ACCESS_MODE_STORE),
      m_field_name(field_name),
      m_target(target),
      m_symbol_type(SymbolType::Builtin::UNDEFINED)
{
}

void AstMember::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    m_target->Visit(visitor, mod);
    
    m_target_type = m_target->GetSymbolType();
    ASSERT(m_target_type != nullptr);

    if (m_target_type != SymbolType::Builtin::ANY) {
        // start looking at the target type,
        // iterate through base type
        SymbolTypePtr_t field_type = nullptr;

        while (field_type == nullptr && m_target_type != nullptr) {
            // allow boxing/unboxing for 'Maybe(T)' type
            if (m_target_type->GetBaseType() != nullptr &&
                m_target_type->GetBaseType()->TypeEqual(*SymbolType::Builtin::MAYBE))
            {
                m_target_type = m_target_type->GetGenericInstanceInfo().m_generic_args[0].m_type;
            }

            ASSERT(m_target_type != nullptr);
            
            field_type = m_target_type->FindMember(m_field_name);

            if (field_type == nullptr) {
                m_target_type = m_target_type->GetBaseType();
            } else {
                break;
            }
        }

        if (field_type != nullptr) {
            m_symbol_type = field_type;
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_not_a_data_member,
                m_location,
                m_field_name,
                m_target->GetSymbolType()->GetName()
            ));
        }
    } else {
        m_symbol_type = SymbolType::Builtin::ANY;
    }
}

std::unique_ptr<Buildable> AstMember::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    ASSERT(m_target != nullptr);
    chunk->Append(m_target->Build(visitor, mod));

    ASSERT(m_target_type != nullptr);

    if (m_target_type == SymbolType::Builtin::ANY) {
        // for Any type we will have to load from hash
        const uint32_t hash = hash_fnv_1(m_field_name.c_str());

        switch (m_access_mode) {
            case ACCESS_MODE_LOAD:
                chunk->Append(Compiler::LoadMemberFromHash(visitor, mod, hash));
                break;
            case ACCESS_MODE_STORE:
                chunk->Append(Compiler::StoreMemberFromHash(visitor, mod, hash));
                break;
        }
    } else {
        int found_index = -1;

        // get member index from name
        for (size_t i = 0; i < m_target_type->GetMembers().size(); i++) {
            if (std::get<0>(m_target_type->GetMembers()[i]) == m_field_name) {
                found_index = i;
                break;
            }
        }

        if (found_index != -1) {
            switch (m_access_mode) {
                case ACCESS_MODE_LOAD:
                    // just load the data member.
                    chunk->Append(Compiler::LoadMemberAtIndex(
                        visitor,
                        mod,
                        found_index
                    ));
                    break;
                case ACCESS_MODE_STORE:
                    // we are in storing mode, so store to LAST item in the member expr.
                    chunk->Append(Compiler::StoreMemberAtIndex(
                        visitor,
                        mod,
                        found_index
                    ));
                    break;
            }
        }
    }

    return std::move(chunk);
}

void AstMember::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Optimize(visitor, mod);

    // TODO: check if the member being accessed is constant and can
    // be optimized
}

Pointer<AstStatement> AstMember::Clone() const
{
    return CloneImpl();
}

int AstMember::IsTrue() const
{
    return -1;
}

bool AstMember::MayHaveSideEffects() const
{
    return false;
}

SymbolTypePtr_t AstMember::GetSymbolType() const
{
    ASSERT(m_symbol_type != nullptr);
    return m_symbol_type;
}
