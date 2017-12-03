#ifndef AST_SYMBOL_QUERY
#define AST_SYMBOL_QUERY

#include <string>

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/type-system/SymbolType.hpp>

class AstSymbolQuery : public AstExpression {
public:
    AstSymbolQuery(
      const std::string &command_name,
      const std::shared_ptr<AstExpression> &expr,
      const SourceLocation &location);
    virtual ~AstSymbolQuery() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetExprType() const override;
    virtual const AstExpression *GetValueOf() const override;

private:
    std::string m_command_name;
    std::shared_ptr<AstExpression> m_expr;

    // set while analyzing
    SymbolTypePtr_t m_symbol_type;
    std::shared_ptr<AstExpression> m_result_value;

    inline Pointer<AstSymbolQuery> CloneImpl() const
    {
        return Pointer<AstSymbolQuery>(new AstSymbolQuery(
            m_command_name,
            CloneAstNode(m_expr),
            m_location
        ));
    }
};

#endif