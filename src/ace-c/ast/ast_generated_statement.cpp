#include <ace-c/ast/ast_generated_statement.hpp>

AstGeneratedStatement::AstGeneratedStatement(CompilerFunction_t on_visit,
    CompilerFunction_t on_build,
    CompilerFunction_t on_optimize,
    const SourceLocation &location)
    : AstStatement(location),
      m_on_visit(on_visit),
      m_on_build(on_build),
      m_on_optimize(on_optimize)
{
}

void AstGeneratedStatement::Visit(AstVisitor *visitor, Module *mod)
{
    if (m_on_visit) {
        m_on_visit(visitor, mod);
    }
}

void AstGeneratedStatement::Build(AstVisitor *visitor, Module *mod)
{
    if (m_on_build) {
        m_on_build(visitor, mod);
    }
}

void AstGeneratedStatement::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_on_optimize) {
        m_on_optimize(visitor, mod);
    }
}