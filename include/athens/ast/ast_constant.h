#ifndef AST_CONSTANT_H
#define AST_CONSTANT_H

#include <athens/ast/ast_expression.h>
#include <athens/types.h>

class AstConstant : public AstExpression {
public:
    AstConstant(const SourceLocation &location);
    virtual ~AstConstant() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) = 0;
    virtual void Optimize(AstVisitor *visitor) override;
    virtual int IsTrue() const = 0;
    virtual bool IsNumber() const = 0;
    virtual a_int IntValue() const = 0;
    virtual a_float FloatValue() const = 0;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator-(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator*(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator/(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator%(
        const std::shared_ptr<AstConstant> &right) const = 0;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator&(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator|(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator<<(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator>>(
        const std::shared_ptr<AstConstant> &right) const = 0;

    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(
        const std::shared_ptr<AstConstant> &right) const = 0;
    virtual std::shared_ptr<AstConstant> operator||(
        const std::shared_ptr<AstConstant> &right) const = 0;
};

#endif
