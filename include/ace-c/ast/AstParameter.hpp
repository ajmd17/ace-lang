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
    virtual Pointer<AstStatement> Clone() const override;

    bool IsVariadic() const { return m_is_variadic; }

private:
    bool m_is_variadic;
    std::shared_ptr<AstTypeSpecification> m_type_spec;

    inline Pointer<AstParameter> CloneImpl() const
    {
        return Pointer<AstParameter>(new AstParameter(
            m_name,
            CloneAstNode(m_type_spec),
            m_is_variadic,
            m_location));
    }
};

#endif
