#include <ace-c/ast/AstBlockExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstReturnStatement.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/SemanticAnalyzer.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <iostream>

AstBlockExpression::AstBlockExpression(
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_block(block),
      m_num_locals(0),
      m_last_is_return(false)
{
}

void AstBlockExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);
    //m_block->Visit(visitor, mod);

    // hold a vector of all declarations in the block.
    // at the end of the block, create an Object declaration where all members are
    // assigned to the found declarations (members will also have the same name).
    // note, that they are not assigned to the same expression.
    // it will be assigned to the variable itself, so that expressions are only evaluated once.
    std::vector<SymbolMember_t> member_types;
    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;
    //std::vector<std::shared_ptr<AstVariableDeclaration>> block_members;
    std::vector<std::shared_ptr<AstVariable>> block_member_refs;

    // open the new scope
    mod->m_scopes.Open(Scope());

    for (auto &child : m_block->GetChildren()) {
        ASSERT(child != nullptr);

        child->Visit(visitor, mod);

        if (auto *decl = dynamic_cast<AstDeclaration*>(child.get())) {
            /*block_members.push_back(std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
                decl->GetName(),
                nullptr,
                std::shared_ptr<AstVariable>(new AstVariable(
                    decl->GetName() // same name
                    decl->GetLocation()
                )),
                false,
                decl->GetLocation()
            )));*/

            block_member_refs.push_back(std::shared_ptr<AstVariable>(new AstVariable(
                decl->GetName(), // same name
                decl->GetLocation()
            )));
        }
    }

    for (auto &member : block_member_refs) {
        ASSERT(member != nullptr);
        member->Visit(visitor, mod);

        member_types.push_back(std::make_tuple(
            member->GetName(),
            member->GetSymbolType(),
            member
        ));

        generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
            member->GetName(),
            member->GetSymbolType(),
            nullptr
        });
    }
    
    m_last_is_return = !(m_block->GetChildren().empty()) &&
        (dynamic_cast<AstReturnStatement*>(m_block->GetChildren().back().get()) != nullptr);

    // store number of locals, so we can pop them from the stack later
    Scope &this_scope = mod->m_scopes.Top();
    m_num_locals = this_scope.GetIdentifierTable().CountUsedVariables();

    // go down to previous scope
    mod->m_scopes.Close();

    SymbolTypePtr_t symbol_type_base = SymbolType::GenericInstance(
        BuiltinTypes::BLOCK_TYPE, 
        GenericInstanceTypeInfo {
            generic_param_types
        }
    );

    visitor->GetCompilationUnit()->GetCurrentModule()->
        m_scopes.Root().GetIdentifierTable().AddSymbolType(symbol_type_base);

    m_symbol_type = SymbolType::Extend(symbol_type_base, member_types);

    // allow generic instance to be used in code
    visitor->GetCompilationUnit()->GetCurrentModule()->
        m_scopes.Root().GetIdentifierTable().AddSymbolType(m_symbol_type);
    
    // register the type
    visitor->GetCompilationUnit()->RegisterType(m_symbol_type);
}

std::unique_ptr<Buildable> AstBlockExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);

    ASSERT(m_symbol_type != nullptr);
    ASSERT(m_symbol_type->GetDefaultValue() != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // build each child
    for (auto &child : m_block->GetChildren()) {
        ASSERT(child != nullptr);
        chunk->Append(child->Build(visitor, mod));
    }
    
    // build in the created object (this will be the result value obtained)
    chunk->Append(m_symbol_type->GetDefaultValue()->Build(visitor, mod));

    // how many times to pop the stack
    size_t pop_times = 0;

    // pop all local variables off the stack
    for (int i = 0; i < m_num_locals; i++) {
        if (!m_last_is_return) {
            pop_times++;
        }

        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    chunk->Append(Compiler::PopStack(visitor, pop_times));

    return std::move(chunk);
}

void AstBlockExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);

    for (auto &child : m_block->GetChildren()) {
        ASSERT(child != nullptr);
        child->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstBlockExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstBlockExpression::IsTrue() const
{
    return Tribool::True();
}

bool AstBlockExpression::MayHaveSideEffects() const
{
    ASSERT(m_block != nullptr);
    
    for (const auto &child : m_block->GetChildren()) {
        if (AstExpression *expr = dynamic_cast<AstExpression*>(child.get())) {
            if (expr->MayHaveSideEffects()) {
                return true;
            }
        }
    }

    return false;
}

SymbolTypePtr_t AstBlockExpression::GetSymbolType() const
{
    return m_symbol_type;
}
