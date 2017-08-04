#ifndef AST_EXPRESSION_HPP
#define AST_EXPRESSION_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/type-system/SymbolType.hpp>
#include <ace-c/enums.hpp>

#include <common/Tribool.hpp>

class AstTypeObject;

class AstExpression : public AstStatement {
public:
    AstExpression(const SourceLocation &location,
        int access_options);
    virtual ~AstExpression() = default;

    inline int GetAccessOptions() const
        { return m_access_options; }

    inline AccessMode GetAccessMode() const
      { return m_access_mode; }
    inline void SetAccessMode(AccessMode access_mode)
      { m_access_mode = access_mode; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override = 0;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;
    
    virtual Pointer<AstStatement> Clone() const override = 0;

    /** Determine whether the expression would evaluate to true.
        Returns -1 if it cannot be evaluated at compile time.
    */
    virtual Tribool IsTrue() const = 0;
    inline int IsFalse() const { int t = IsTrue(); return (t == -1) ? t : !t; }
    /** Determine whether or not there is a possibility of side effects. */
    virtual bool MayHaveSideEffects() const = 0;
    virtual std::shared_ptr<AstTypeObject> GetTypeOf() const = 0;

    inline SymbolTypePtr_t GetExprType() const
    {
        if (std::shared_ptr<AstTypeObject> type_object = this->GetTypeOf()) {
            return type_object->GetHeldType();
        }

        return nullptr;
    }
    //virtual SymbolTypePtr_t GetExprType() const = 0;

    bool m_is_standalone;

protected:
    AccessMode m_access_mode;
    int m_access_options;
};

#endif
