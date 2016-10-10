#ifndef AST_TRUE_H
#define AST_TRUE_H

#include <athens/ast/ast_constant.h>

class AstTrue : public AstConstant {
public:
    AstTrue(const SourceLocation &location);

    virtual void Build(AstVisitor *visitor) override;
    virtual int IsTrue() const override;
    virtual bool IsNumber() const override;
    virtual a_int IntValue() const override;
    virtual a_float FloatValue() const override;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator-(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator*(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator/(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator%(
        const std::shared_ptr<AstConstant> &right) const override;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator&(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator|(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator<<(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator>>(
        const std::shared_ptr<AstConstant> &right) const override;
    
    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(
        const std::shared_ptr<AstConstant> &right) const override;
    virtual std::shared_ptr<AstConstant> operator||(
        const std::shared_ptr<AstConstant> &right) const override;
};

#endif