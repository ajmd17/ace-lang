#ifndef AST_CONSTANT_HPP
#define AST_CONSTANT_HPP

#include <athens/ast/ast_expression.hpp>
#include <athens/types.hpp>

class AstConstant : public AstExpression {
public:
    AstConstant(const SourceLocation &location);
    virtual ~AstConstant() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override = 0;
    virtual void Optimize(AstVisitor *visitor) override;
    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override;
    virtual bool IsNumber() const = 0;
    virtual a_int IntValue() const = 0;
    virtual a_float FloatValue() const = 0;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator-(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator*(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator/(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator%(
        AstConstant *right) const = 0;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator&(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator|(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator<<(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator>>(
        AstConstant *right) const = 0;

    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(
        AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator||(
        AstConstant *right) const = 0;

    virtual std::shared_ptr<AstConstant> Equals(AstConstant *right) const = 0;
};

#endif
