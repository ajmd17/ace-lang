#ifndef AST_VOID_H
#define AST_VOID_H

#include <athens/ast/ast_constant.h>

class AstVoid : public AstConstant {
public:
    AstVoid(const SourceLocation &location);

    virtual void Build(AstVisitor *visitor) const;
    virtual int IsTrue() const;
    virtual a_int IntValue() const;
    virtual a_float FloatValue() const;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator-(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator*(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator/(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator%(
        const std::shared_ptr<AstConstant> &right) const;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator&(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator|(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator<<(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator>>(
        const std::shared_ptr<AstConstant> &right) const;
    
    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(
        const std::shared_ptr<AstConstant> &right) const;
    virtual std::shared_ptr<AstConstant> operator||(
        const std::shared_ptr<AstConstant> &right) const;
};

#endif