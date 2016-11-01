#ifndef AST_FUNCTION_CALL_HPP
#define AST_FUNCTION_CALL_HPP

#include <athens/ast/ast_identifier.hpp>

#include <string>
#include <vector>
#include <memory>

class AstFunctionCall : public AstIdentifier {
public:
    AstFunctionCall(const std::string &name,
        const std::vector<std::shared_ptr<AstExpression>> &args,
        const SourceLocation &location);
    virtual ~AstFunctionCall() = default;

    inline void AddArgumentToFront(const std::shared_ptr<AstExpression> &arg)
        { m_args.insert(m_args.begin(), arg); }
    inline void AddArgument(const std::shared_ptr<AstExpression> &arg)
        { m_args.push_back(arg); }
    inline const std::vector<std::shared_ptr<AstExpression>> &GetArguments() const { return m_args; }
    inline bool HasSelfObject() const { return m_has_self_object; }
    inline void SetHasSelfObject(bool has_self_object) { m_has_self_object = has_self_object; }

    void BuildArgumentsStart(AstVisitor *visitor, Module *mod);
    void BuildArgumentsEnd(AstVisitor *visitor, Module *mod);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

protected:
    std::vector<std::shared_ptr<AstExpression>> m_args;
    ObjectType m_return_type;
    bool m_has_self_object;
};

#endif
