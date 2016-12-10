#ifndef AST_IDENTIFIER_HPP
#define AST_IDENTIFIER_HPP

#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/identifier.hpp>

#include <string>

enum AccessMode {
    ACCESS_MODE_LOAD,
    ACCESS_MODE_STORE
};

class AstIdentifier : public AstExpression {
public:
    AstIdentifier(const std::string &name, const SourceLocation &location);
    virtual ~AstIdentifier() = default;

    void PerformLookup(AstVisitor *visitor, Module *mod);

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }
    inline void SetIdentifier(Identifier *identifier) { m_identifier = identifier; }
    inline AccessMode GetAccessMode() const { return m_access_mode; }
    inline void SetAccessMode(AccessMode access_mode) { m_access_mode = access_mode; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;

    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override = 0;
    virtual ObjectType GetObjectType() const override;

protected:
    std::string m_name;
    Identifier *m_identifier;
    AccessMode m_access_mode;
    bool m_in_function;

    int GetStackOffset(int stack_size) const;
};

#endif
