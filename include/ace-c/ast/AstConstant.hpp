#ifndef AST_CONSTANT_HPP
#define AST_CONSTANT_HPP

#include <ace-c/ast/AstExpression.hpp>

#include <common/typedefs.hpp>

class AstConstant : public AstExpression {
public:
    AstConstant(const SourceLocation &location);
    virtual ~AstConstant() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override = 0;
    virtual Pointer<AstStatement> Clone() const override = 0;

    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override;
    virtual bool IsNumber() const = 0;
    virtual ace::aint32 IntValue() const = 0;
    virtual ace::afloat32 FloatValue() const = 0;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator-(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator*(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator/(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator%(AstConstant *right) const = 0;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator&(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator|(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator<<(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator>>(AstConstant *right) const = 0;

    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator||(AstConstant *right) const = 0;

    // Comparison operators
    virtual std::shared_ptr<AstConstant> operator<(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator>(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator<=(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> operator>=(AstConstant *right) const = 0;
    virtual std::shared_ptr<AstConstant> Equals(AstConstant *right) const = 0;

    // Unary operators
    virtual std::shared_ptr<AstConstant> operator-() const = 0;
    virtual std::shared_ptr<AstConstant> operator~() const = 0;
    virtual std::shared_ptr<AstConstant> operator!() const = 0;
};

#endif
