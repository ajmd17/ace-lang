#ifndef AST_CALL_EXPRESSION_HPP
#define AST_CALL_EXPRESSION_HPP

#include <ace-c/ast/AstIdentifier.hpp>
#include <ace-c/ast/AstArgument.hpp>

#include <string>
#include <vector>
#include <memory>

class AstCallExpression : public AstExpression {
public:
    AstCallExpression(
        const std::shared_ptr<AstExpression> &target,
        const std::vector<std::shared_ptr<AstArgument>> &args,
        const SourceLocation &location);
    virtual ~AstCallExpression() = default;

    inline void AddArgumentToFront(const std::shared_ptr<AstArgument> &arg)
        { m_args.insert(m_args.begin(), arg); }
    inline void AddArgument(const std::shared_ptr<AstArgument> &arg)
        { m_args.push_back(arg); }
    inline const std::vector<std::shared_ptr<AstArgument>> &GetArguments() const
        { return m_args; }

    inline void SetArgumentOrdering(const std::vector<int> &arg_ordering)
        { m_arg_ordering = arg_ordering; }
    inline std::vector<int> GetArgumentOrdering() const
        { return m_arg_ordering; }
    
    inline const SymbolTypePtr_t &GetReturnType() const
        { return m_return_type; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

protected:
    std::shared_ptr<AstExpression> m_target;
    std::vector<std::shared_ptr<AstArgument>> m_args;

    // set while analyzing
    bool m_is_method_call;
    std::vector<int> m_arg_ordering;
    SymbolTypePtr_t m_return_type;

    inline Pointer<AstCallExpression> CloneImpl() const
    {
        return Pointer<AstCallExpression>(
            new AstCallExpression(
                CloneAstNode(m_target),
                CloneAllAstNodes(m_args),
                m_location
            ));
    }
};

#endif
