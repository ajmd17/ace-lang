#include <athens/ast/ast_import.h>
#include <athens/ast_visitor.h>

AstImport::AstImport(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstImport::Visit(AstVisitor *visitor)
{
    visitor->GetCompilationUnit()->m_modules.push_back(LoadModule(
        visitor->GetCompilationUnit()));
}

void AstImport::Build(AstVisitor *visitor) const
{
}
