#include <ace-c/ast/AstBlockExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstAliasDeclaration.hpp>
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
#if 0
    // block-expr is actually just an immediately invoked closure
    ASSERT(m_block != nullptr);

    m_call_expr.reset(new AstCallExpression(
        std::shared_ptr<AstFunctionExpression>(new AstFunctionExpression(
            {},
            nullptr,
            m_block,
            false,
            false,
            false,
            m_location
        )),
        {},
        false,
        m_location
    ));

    m_call_expr->Visit(visitor, mod);

#else
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
    std::vector<std::tuple<std::string, std::shared_ptr<AstVariable>>> block_member_refs;

    // open the new scope
    mod->m_scopes.Open(Scope());

    m_children.reserve(m_block->GetChildren().size());

    for (auto &child : m_block->GetChildren()) {
        ASSERT(child != nullptr);

        m_children.push_back(child);

        if (auto *decl = dynamic_cast<AstVariableDeclaration*>(child.get())) {
            // rewrite name of variable.
            // prefix the original with $__ to hide it,
            // and create an alias with the original name pointing to this.<var name>
            const std::string var_name = decl->GetName();
            /*decl->SetName(std::string("$__") + var_name);

            m_children.push_back(std::shared_ptr<AstAliasDeclaration>(new AstAliasDeclaration(
                var_name,
                std::shared_ptr<AstMember>(new AstMember(
                    var_name,
                    std::shared_ptr<AstVariable>(new AstVariable(
                        "self",
                        m_location
                    )),
                    m_location
                )),
                decl->GetLocation()
            )));*/

            block_member_refs.push_back(std::make_tuple(
                var_name, // original name
                std::shared_ptr<AstVariable>(new AstVariable(
                    /*std::string("$__") +*/ var_name,
                    decl->GetLocation()
                ))
            ));
        }
    }

    for (auto &child : m_children) {
        ASSERT(child != nullptr);
        child->Visit(visitor, mod);
    }

    std::shared_ptr<AstBlock> closure_block(new AstBlock(m_location));

    // add a Closure object that will be the result of the expression
    for (auto &tup : block_member_refs) {
        closure_block->AddChild(std::get<1>(tup));
    }
    
    m_result_closure.reset(new AstFunctionExpression(
        {},
        nullptr,
        closure_block,
        false,
        false,
        false,
        m_location
    ));

    m_result_closure->Visit(visitor, mod);

    /*for (auto &tup : block_member_refs) {
        const std::string &name = std::get<0>(tup);
        auto &expr = std::get<1>(tup);

        ASSERT(expr != nullptr);
        expr->Visit(visitor, mod);

        member_types.push_back(std::make_tuple(
            name,
            expr->GetSymbolType(),
            expr
        ));

        generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
            name,
            expr->GetSymbolType(),
            nullptr
        });
    }*/
    
    m_last_is_return = !(m_children.empty()) &&
        (dynamic_cast<AstReturnStatement*>(m_children.back().get()) != nullptr);

    // store number of locals, so we can pop them from the stack later
    Scope &this_scope = mod->m_scopes.Top();
    m_num_locals = this_scope.GetIdentifierTable().CountUsedVariables();

    // go down to previous scope
    mod->m_scopes.Close();

    /*SymbolTypePtr_t symbol_type_base = SymbolType::GenericInstance(
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
    visitor->GetCompilationUnit()->RegisterType(m_symbol_type);*/
#endif
}

std::unique_ptr<Buildable> AstBlockExpression::Build(AstVisitor *visitor, Module *mod)
{
#if 0
    ASSERT(m_call_expr != nullptr);
    return m_call_expr->Build(visitor, mod);

#else

    //ASSERT(m_symbol_type != nullptr);
    //ASSERT(m_symbol_type->GetDefaultValue() != nullptr);

    ASSERT(m_result_closure != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // build each child
    for (auto &child : m_children) {
        ASSERT(child != nullptr);
        chunk->Append(child->Build(visitor, mod));
    }

    chunk->Append(m_result_closure->Build(visitor, mod));
    
    // build in the created object (this will be the result value obtained)
    //chunk->Append(m_symbol_type->GetDefaultValue()->Build(visitor, mod));

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
#endif
}

void AstBlockExpression::Optimize(AstVisitor *visitor, Module *mod)
{
#if 0
    ASSERT(m_call_expr != nullptr);
    m_call_expr->Optimize(visitor, mod);
    
#else
   // ASSERT(m_block != nullptr);
    ASSERT(m_result_closure != nullptr);

    for (auto &child : m_children) {
        ASSERT(child != nullptr);
        child->Optimize(visitor, mod);
    }

    m_result_closure->Optimize(visitor, mod);

#endif
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
#if 0
    ASSERT(m_call_expr != nullptr);
    return m_call_expr->MayHaveSideEffects();

#else
    //ASSERT(m_block != nullptr);
    
    for (const auto &child : m_children) {
        if (AstExpression *expr = dynamic_cast<AstExpression*>(child.get())) {
            if (expr->MayHaveSideEffects()) {
                return true;
            }
        }
    }

    return false;
#endif
}

SymbolTypePtr_t AstBlockExpression::GetSymbolType() const
{
#if 0
    ASSERT(m_call_expr != nullptr);
    ASSERT(m_call_expr->GetSymbolType() != nullptr);

    return m_call_expr->GetSymbolType();
#else
    //ASSERT(m_symbol_type != nullptr);
    //return m_symbol_type;

    ASSERT(m_result_closure != nullptr);
    ASSERT(m_result_closure->GetSymbolType() != nullptr);

    return m_result_closure->GetSymbolType();
#endif
}
