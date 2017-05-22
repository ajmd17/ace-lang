#ifndef AST_EVENT_HPP
#define AST_EVENT_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstFunctionExpression.hpp>

class AstEvent : public AstExpression {
public:
    AstEvent(const std::string &key,
      const std::shared_ptr<AstFunctionExpression> &trigger,
      const SourceLocation &location);
    virtual ~AstEvent() = default;

    inline const std::string &GetKey() const { return m_key; }
    inline const std::shared_ptr<AstFunctionExpression> &GetTrigger() const
      { return m_trigger; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    std::string m_key;
    std::shared_ptr<AstFunctionExpression> m_trigger;

    inline Pointer<AstEvent> CloneImpl() const
    {
        return Pointer<AstEvent>(new AstEvent(
            m_key,
            CloneAstNode(m_trigger),
            m_location
        ));
    }
};

#endif
