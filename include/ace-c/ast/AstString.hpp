#ifndef AST_STRING_HPP
#define AST_STRING_HPP

#include <ace-c/ast/AstConstant.hpp>

#include <string>

class AstString : public AstConstant {
public:
    AstString(const std::string &value, const SourceLocation &location);

    const std::string &GetValue() const { return m_value; }

    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool IsNumber() const override;
    virtual ace::aint32 IntValue() const override;
    virtual ace::afloat32 FloatValue() const override;
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
    std::string m_value;

    // set while compiling
    int m_static_id;

    inline Pointer<AstString> CloneImpl() const
    {
        return Pointer<AstString>(new AstString(
            m_value,
            m_location
        ));
    }
};

#endif
