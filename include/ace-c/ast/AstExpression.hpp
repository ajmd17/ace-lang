#ifndef AST_EXPRESSION_HPP
#define AST_EXPRESSION_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ObjectType.hpp>
#include <ace-c/SymbolType.hpp>
#include <ace-c/enums.hpp>

class AstExpression : public AstStatement {
public:
    AstExpression(
        const SourceLocation &location,
        int access_options
    );
    virtual ~AstExpression() = default;

    inline const int GetAccessOptions() const
        { return m_access_options; }

    inline AccessMode GetAccessMode() const
      { return m_access_mode; }
    inline void SetAccessMode(AccessMode access_mode)
      { m_access_mode = access_mode; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Recreate(std::ostringstream &ss) override = 0;
    virtual Pointer<AstStatement> Clone() const override = 0;

    /** Determine whether the expression would evaluate to true.
        Returns -1 if it cannot be evaluated at compile time.
    */
    virtual int IsTrue() const = 0;
    inline int IsFalse() const { int t = IsTrue(); return (t == -1) ? t : !t; }
    /** Determine whether or not there is a possibility of side effects. */
    virtual bool MayHaveSideEffects() const = 0;
    virtual SymbolTypePtr_t GetSymbolType() const = 0;

    bool m_is_standalone;

protected:
    AccessMode m_access_mode;
    int m_access_options;
};

#endif
