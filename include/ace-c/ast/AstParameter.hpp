#ifndef AST_PARAMETER_HPP
#define AST_PARAMETER_HPP

#include <ace-c/ast/AstDeclaration.hpp>
#include <ace-c/ast/AstPrototypeSpecification.hpp>
#include <ace-c/ast/AstExpression.hpp>

class AstParameter : public AstDeclaration {
public:
    AstParameter(const std::string &name,
        const std::shared_ptr<AstPrototypeSpecification> &type_spec,
        const std::shared_ptr<AstExpression> &default_param,
        bool is_variadic,
        bool is_const,
        const SourceLocation &location);
    virtual ~AstParameter() = default;

    inline const std::shared_ptr<AstExpression> &GetDefaultValue() const
        { return m_default_param; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    inline bool IsVariadic() const { return m_is_variadic; }
    inline bool IsConst() const { return m_is_const; }

private:
    std::shared_ptr<AstPrototypeSpecification> m_type_spec;
    std::shared_ptr<AstExpression> m_default_param;
    bool m_is_variadic;
    bool m_is_const;

    inline Pointer<AstParameter> CloneImpl() const
    {
        return Pointer<AstParameter>(new AstParameter(
            m_name,
            CloneAstNode(m_type_spec),
            CloneAstNode(m_default_param),
            m_is_variadic,
            m_is_const,
            m_location
        ));
    }
};

#endif
