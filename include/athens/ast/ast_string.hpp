#ifndef AST_STRING_HPP
#define AST_STRING_HPP

#include <athens/ast/ast_constant.hpp>

#include <string>

class AstString : public AstConstant {
public:
    AstString(const std::string &value, const SourceLocation &location);

    virtual void Build(AstVisitor *visitor, Module *mod) override;

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

    virtual std::shared_ptr<AstConstant> Equals(AstConstant *right) const override;

private:
    std::string m_value;

    // set while compiling
    int m_static_id;
};

#endif
