#ifndef AST_FUNCTION_EXPRESSION_HPP
#define AST_FUNCTION_EXPRESSION_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstParameter.hpp>
#include <ace-c/ast/AstBlock.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>

#include <memory>
#include <vector>

class AstFunctionExpression : public AstExpression {
public:
    AstFunctionExpression(const std::vector<std::shared_ptr<AstParameter>> &parameters,
        const std::shared_ptr<AstTypeSpecification> &type_specification,
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location);
    virtual ~AstFunctionExpression() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;
    
    inline const ObjectType &GetReturnType() const { return m_return_type; }
    inline void SetReturnType(const ObjectType &return_type) { m_return_type = return_type; }

protected:
    std::vector<std::shared_ptr<AstParameter>> m_parameters;
    std::shared_ptr<AstTypeSpecification> m_type_specification;
    std::shared_ptr<AstBlock> m_block;
    ObjectType m_object_type;
    ObjectType m_return_type;

    SymbolTypePtr_t m_symbol_type;

    // set while compiling
    int m_static_id;
};

#endif
