#ifndef AST_ARGUMENT_LIST_HPP
#define AST_ARGUMENT_LIST_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/SymbolType.hpp>

#include <string>

class AstArgumentList : public AstExpression {
public:
    AstArgumentList(const std::vector<std::shared_ptr<AstArgument>> &args,
      const SourceLocation &location);
    virtual ~AstArgumentList() = default;

    inline const std::vector<std::shared_ptr<AstArgument>> &GetArguments() const
      { return m_args; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;
    
private:
    std::vector<std::shared_ptr<AstArgument>> m_args;

    inline Pointer<AstArgumentList> CloneImpl() const
    {
        return Pointer<AstArgumentList>(new AstArgumentList(
            CloneAllAstNodes(m_args),
            m_location
        ));
    }
};

#endif
