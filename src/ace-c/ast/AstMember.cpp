#include <ace-c/ast/AstMember.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstFunctionCall.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

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
    
    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    if (target_type != SymbolType::Builtin::ANY) {
        if (const SymbolTypePtr_t field_type = target_type->FindMember(m_field_name)) {
            m_symbol_type = field_type;
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                Level_fatal,
                Msg_not_a_data_member,
                m_location,
                m_field_name,
                target_type->GetName()
            ));
        }
    } else {
        m_symbol_type = SymbolType::Builtin::ANY;
    }
}

void AstMember::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    m_target->Build(visitor, mod);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    if (target_type == SymbolType::Builtin::ANY) {
        // for Any type we will have to load from hash
        uint32_t hash = hash_fnv_1(m_field_name.c_str());

        Compiler::LoadMemberFromHash(visitor, mod, hash);

        // todo: StoreMemberFromHash
    } else {
        std::pair<int, SymbolTypePtr_t> dm = { -1, nullptr };

        // get member index from name
        for (size_t i = 0; i < target_type->GetMembers().size(); i++) {
            if (std::get<0>(target_type->GetMembers()[i]) == m_field_name) {
                dm = {
                    i,
                    std::get<1>(target_type->GetMembers()[i])
                };
                break;
            }
        }

        if (dm.first != -1) {
            std::cout << "m_access_mode = " << m_access_mode << "\n";
            if (m_access_mode == ACCESS_MODE_LOAD) {
                // just load the data member.
                Compiler::LoadMemberAtIndex(visitor, mod, dm.first);
            } else if (m_access_mode == ACCESS_MODE_STORE) {
                // we are in storing mode, so store to LAST item in the member expr.
                Compiler::StoreMemberAtIndex(visitor, mod, dm.first);
            }
        }
    }
}

void AstMember::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Optimize(visitor, mod);

    // TODO: check if the member being accessed is constant and can
    // be optimized
}

void AstMember::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);

    m_target->Recreate(ss);
    ss << ".";
    ss << m_field_name;
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
    return m_symbol_type;
}
