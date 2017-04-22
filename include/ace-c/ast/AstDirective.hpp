#ifndef AST_DIRECTIVE_HPP
#define AST_DIRECTIVE_HPP

#include <ace-c/ast/AstStatement.hpp>

#include <memory>
#include <string>

class AstDirective : public AstStatement {
public:
    AstDirective(const std::string &key, const std::string &value,
      const SourceLocation &location);
    virtual ~AstDirective() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::string m_key;
    std::string m_value;

    inline Pointer<AstDirective> CloneImpl() const
    {
        return Pointer<AstDirective>(new AstDirective(
            m_key,
            m_value,
            m_location));
    }
};

#endif
