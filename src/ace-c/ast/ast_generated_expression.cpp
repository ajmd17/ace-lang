#include <ace-c/ast/ast_generated_expression.hpp>

AstGeneratedExpression::AstGeneratedExpression(CompilerFunction_t on_visit,
    CompilerFunction_t on_build,
    CompilerFunction_t on_optimize,
    const SourceLocation &location)
    : AstExpression(location),
      m_on_visit(on_visit),
      m_on_build(on_build),
      m_on_optimize(on_optimize)
{
}

void AstGeneratedExpression::Visit(AstVisitor *visitor, Module *mod)
{
    if (m_on_visit) {
        m_on_visit(visitor, mod);
    }
}

void AstGeneratedExpression::Build(AstVisitor *visitor, Module *mod)
{
    if (m_on_build) {
        m_on_build(visitor, mod);
    }
}

void AstGeneratedExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_on_optimize) {
        m_on_optimize(visitor, mod);
    }
}

void AstGeneratedExpression::Recreate(std::ostringstream &ss)
{
    ss << "??";
}