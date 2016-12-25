#ifndef AST_IDENTIFIER_HPP
#define AST_IDENTIFIER_HPP

#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/identifier.hpp>
#include <ace-c/enums.hpp>

#include <string>

struct AstIdentifierProperties {
    Identifier *identifier = nullptr;

    AccessMode access_mode = ACCESS_MODE_LOAD;
    IdentifierType identifier_type = IDENTIFIER_TYPE_UNKNOWN;

    bool is_in_function = false;
    int depth = 0;
};

class AstIdentifier : public AstExpression {
public:
    AstIdentifier(const std::string &name, const SourceLocation &location);
    virtual ~AstIdentifier() = default;

    void PerformLookup(AstVisitor *visitor, Module *mod);
    void CheckInFunction(AstVisitor *visitor, Module *mod);

    inline const std::string &GetName() const { return m_name; }
    inline AstIdentifierProperties &GetProperties() { return m_properties; }
    inline const AstIdentifierProperties &GetProperties() const { return m_properties; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Recreate(std::ostringstream &ss) override = 0;

    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override = 0;
    virtual ObjectType GetObjectType() const override;

protected:
    std::string m_name;
    
    AstIdentifierProperties m_properties;

    int GetStackOffset(int stack_size) const;
};

#endif
