#ifndef AST_VARIABLE_DECLARATION_HPP
#define AST_VARIABLE_DECLARATION_HPP

#include <ace-c/ast/AstDeclaration.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/ast/AstPrototypeSpecification.hpp>
#include <ace-c/type-system/SymbolType.hpp>

#include <memory>

class AstVariableDeclaration : public AstDeclaration {
public:
    AstVariableDeclaration(const std::string &name,
        const std::shared_ptr<AstPrototypeSpecification> &proto,
        //const std::shared_ptr<AstTypeSpecification> &type_specification,
        const std::shared_ptr<AstExpression> &assignment,
        bool is_const,
        const SourceLocation &location);
    virtual ~AstVariableDeclaration() = default;

    inline const std::shared_ptr<AstExpression> &GetAssignment() const
        { return m_assignment; }
    inline bool IsConst() const { return m_is_const; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::shared_ptr<AstPrototypeSpecification> m_proto;
    //std::shared_ptr<AstTypeSpecification> m_type_specification;
    std::shared_ptr<AstExpression> m_assignment;
    bool m_is_const;

    // set while analyzing
    bool m_assignment_already_visited;
    std::shared_ptr<AstExpression> m_real_assignment;

    SymbolTypeWeakPtr_t m_symbol_type;

    inline Pointer<AstVariableDeclaration> CloneImpl() const
    {
        return Pointer<AstVariableDeclaration>(new AstVariableDeclaration(
            m_name,
            CloneAstNode(m_proto),
            //CloneAstNode(m_type_specification),
            CloneAstNode(m_assignment),
            m_is_const,
            m_location
        ));
    }
};

#endif
