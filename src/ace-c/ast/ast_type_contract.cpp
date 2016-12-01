#include <ace-c/ast/ast_type_contract.hpp>

AstTypeContractExpression::AstTypeContractExpression(const SourceLocation &location)
    : AstStatement(location)
{
}


AstTypeContractTerm::AstTypeContractTerm(const std::string &type_contract_operation,
    const std::shared_ptr<AstTypeSpecification> &type_spec,
    const SourceLocation &location)
    : AstTypeContractExpression(location),
      m_type_contract_operation(type_contract_operation),
      m_type_spec(type_spec)
{
}

void AstTypeContractTerm::Visit(AstVisitor *visitor, Module *mod)
{
    m_type_spec->Visit(visitor, mod);
    // TODO
}

void AstTypeContractTerm::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeContractTerm::Optimize(AstVisitor *visitor, Module *mod)
{
}


AstTypeContractBinaryExpression::AstTypeContractBinaryExpression(
    const std::shared_ptr<AstTypeContractExpression> &left,
    const std::shared_ptr<AstTypeContractExpression> &right,
    const Operator *op,
    const SourceLocation &location)
    : AstTypeContractExpression(location),
      m_left(left),
      m_right(right),
      m_op(op)
{
}

void AstTypeContractBinaryExpression::Visit(AstVisitor *visitor, Module *mod)
{
    // TODO
    m_left->Visit(visitor, mod);
    m_right->Visit(visitor, mod);
}

void AstTypeContractBinaryExpression::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeContractBinaryExpression::Optimize(AstVisitor *visitor, Module *mod)
{
}