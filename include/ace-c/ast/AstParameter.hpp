#ifndef AST_PARAMETER_HPP
#define AST_PARAMETER_HPP

#include <ace-c/ast/AstDeclaration.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>

class AstParameter : public AstDeclaration {
public:
    AstParameter(const std::string &name,
        const std::shared_ptr<AstTypeSpecification> &type_spec,
        bool is_variadic,
        const SourceLocation &location);
    virtual ~AstParameter() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

    bool IsVariadic() const { return m_is_variadic; }

#if 0
    inline const std::shared_ptr<AstTypeContractExpression> &GetTypeContract() const
        { return m_type_contract; }
    inline void SetTypeContract(const std::shared_ptr<AstTypeContractExpression> &type_contract)
        { m_type_contract = type_contract; }
    inline bool HasTypeContract() const { return m_type_contract != nullptr; }
#endif

private:
    bool m_is_variadic;
    std::shared_ptr<AstTypeSpecification> m_type_spec;
};

#endif
