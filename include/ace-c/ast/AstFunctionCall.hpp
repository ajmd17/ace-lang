#ifndef AST_FUNCTION_CALL_HPP
#define AST_FUNCTION_CALL_HPP

#include <ace-c/ast/AstIdentifier.hpp>
#include <ace-c/ast/AstArgument.hpp>

#include <string>
#include <vector>
#include <memory>

class AstFunctionCall : public AstIdentifier {
public:
    AstFunctionCall(const std::string &name,
        const std::vector<std::shared_ptr<AstArgument>> &args,
        const SourceLocation &location);
    virtual ~AstFunctionCall() = default;

    inline void AddArgumentToFront(const std::shared_ptr<AstArgument> &arg)
        { m_args.insert(m_args.begin(), arg); }
    inline void AddArgument(const std::shared_ptr<AstArgument> &arg)
        { m_args.push_back(arg); }
    inline const std::vector<std::shared_ptr<AstArgument>> &GetArguments() const { return m_args; }

    inline void SetArgumentOrdering(const std::vector<int> &arg_ordering)
        { m_arg_ordering = arg_ordering; }
    
    inline bool HasSelfObject() const { return m_has_self_object; }
    inline void SetHasSelfObject(bool has_self_object) { m_has_self_object = has_self_object; }

    inline const SymbolTypePtr_t &GetReturnType() const { return m_return_type; }

    void BuildArgumentsStart(AstVisitor *visitor, Module *mod);
    void BuildArgumentsEnd(AstVisitor *visitor, Module *mod);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

protected:
    std::vector<std::shared_ptr<AstArgument>> m_args;

    // set while analyzing
    std::vector<int> m_arg_ordering;
    SymbolTypePtr_t m_return_type;
    bool m_has_self_object;

    inline Pointer<AstFunctionCall> CloneImpl() const
    {
        return Pointer<AstFunctionCall>(
            new AstFunctionCall(m_name,
                CloneAllAstNodes(m_args),
                m_location));
    }
};

#endif
