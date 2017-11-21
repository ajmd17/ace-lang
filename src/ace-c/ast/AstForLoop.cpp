#include <ace-c/ast/AstForLoop.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstForLoop::AstForLoop(const std::vector<std::shared_ptr<AstParameter>> &params,
    const std::shared_ptr<AstExpression> &iteree,
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstStatement(location),
      m_params(params),
      m_iteree(iteree),
      m_block(block)
{
}

void AstForLoop::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_iteree != nullptr);

    std::vector<std::shared_ptr<AstArgument>> action_args = {
        std::shared_ptr<AstArgument>(new AstArgument(
            m_iteree,
            false,
            false,
            "",
            m_iteree->GetLocation()
        )),
        std::shared_ptr<AstArgument>(new AstArgument(
            std::shared_ptr<AstFunctionExpression>(new AstFunctionExpression(
                m_params,
                nullptr,
                m_block,
                false,
                false,
                false,
                m_location
            )),
            false,
            false,
            "",
            m_location
        )),
    };

    m_expr = visitor->GetCompilationUnit()->GetAstNodeBuilder()
        .Module("events")
        .Function("call_action")
        .Call(action_args);

    m_expr->Visit(visitor, mod);
}

std::unique_ptr<Buildable> AstForLoop::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    return m_expr->Build(visitor, mod);
}

void AstForLoop::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);
}

Pointer<AstStatement> AstForLoop::Clone() const
{
    return CloneImpl();
}
