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
      m_type_spec(type_spec),
      m_type(TypeContract::Type::TC_INVALID)
{
}

void AstTypeContractTerm::Visit(AstVisitor *visitor, Module *mod)
{
    // check that it is a valid type contract!
    m_type = TypeContract::FromString(m_type_contract_operation);
    if (m_type != TypeContract::Type::TC_INVALID) {
        m_type_spec->Visit(visitor, mod);
    } else {
        // whoops! invalid requirement
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_unknown_type_contract_requirement,
                m_location, m_type_contract_operation));
    }
}

void AstTypeContractTerm::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeContractTerm::Optimize(AstVisitor *visitor, Module *mod)
{
}

void AstTypeContractTerm::Recreate(std::ostringstream &ss)
{
    ss << "??";
}

bool AstTypeContractTerm::Satisfies(AstVisitor *visitor, const ObjectType &object_type) const
{
    switch (m_type) {
    case TypeContract::Type::TC_IS:
        return object_type == m_type_spec->GetObjectType();
    default:
        return false;
    }
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

void AstTypeContractBinaryExpression::Recreate(std::ostringstream &ss)
{
    ss << "??";
}

bool AstTypeContractBinaryExpression::Satisfies(AstVisitor *visitor, const ObjectType &object_type) const
{
    if (m_op == &Operator::operator_bitwise_and) {
        return m_left->Satisfies(visitor, object_type) && m_right->Satisfies(visitor, object_type);
    } else if (m_op == &Operator::operator_bitwise_or) {
        return m_left->Satisfies(visitor, object_type) || m_right->Satisfies(visitor, object_type);
    }
    return false;
}