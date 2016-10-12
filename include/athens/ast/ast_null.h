#ifndef AST_NULL_H
#define AST_NULL_H

#include <athens/ast/ast_constant.h>

class AstNull : public AstConstant {
public:
    AstNull(const SourceLocation &location);

    virtual void Build(AstVisitor *visitor) override;
    virtual int IsTrue() const override;
    virtual bool IsNumber() const override;
    virtual a_int IntValue() const override;
    virtual a_float FloatValue() const override;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator-(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator*(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator/(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator%(
        AstConstant *right) const override;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator&(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator|(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator<<(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator>>(
        AstConstant *right) const override;
    
    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(
        AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator||(
        AstConstant *right) const override;
};

#endif