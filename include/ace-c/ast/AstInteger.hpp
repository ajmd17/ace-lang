#ifndef AST_INTEGER_HPP
#define AST_INTEGER_HPP

#include <ace-c/ast/AstConstant.hpp>

#include <cstdint>

class AstInteger : public AstConstant {
public:
    AstInteger(a_int value, const SourceLocation &location);

    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

    virtual int IsTrue() const override;
    virtual ObjectType GetObjectType() const override;
    virtual bool IsNumber() const override;
    virtual a_int IntValue() const override;
    virtual a_float FloatValue() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

    // Arithmetic operators
    virtual std::shared_ptr<AstConstant> operator+(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator-(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator*(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator/(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator%(AstConstant *right) const override;

    // Bitwise operators
    virtual std::shared_ptr<AstConstant> operator^(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator&(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator|(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator<<(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator>>(AstConstant *right) const override;

    // Logical operators
    virtual std::shared_ptr<AstConstant> operator&&(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator||(AstConstant *right) const override;

    // Comparison operators
    virtual std::shared_ptr<AstConstant> operator<(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator>(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator<=(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> operator>=(AstConstant *right) const override;
    virtual std::shared_ptr<AstConstant> Equals(AstConstant *right) const override;
    
    // Unary operators
    virtual std::shared_ptr<AstConstant> operator-() const override;
    virtual std::shared_ptr<AstConstant> operator~() const override;
    virtual std::shared_ptr<AstConstant> operator!() const override;

private:
    a_int m_value;
};

#endif
