#ifndef AST_MEMBER_ACCESS_HPP
#define AST_MEMBER_ACCESS_HPP

#include <ace-c/ast/AstIdentifier.hpp>

#include <string>
#include <vector>
#include <memory>

class AstMemberAccess : public AstExpression {
public:
    AstMemberAccess(const std::shared_ptr<AstExpression> &target,
        const std::vector<std::shared_ptr<AstIdentifier>> &parts,
        const SourceLocation &location);
    virtual ~AstMemberAccess() = default;

    inline const std::shared_ptr<AstIdentifier> &GetLast() const { return m_parts.back(); }
    inline AccessMode GetAccessMode() const { return m_access_mode; }
    inline void SetAccessMode(AccessMode access_mode) { m_access_mode = access_mode; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

private:
    std::shared_ptr<AstExpression> m_target;
    std::vector<std::shared_ptr<AstIdentifier>> m_parts;
    Module *m_mod_access;
    std::vector<ObjectType> m_part_object_types;
    AccessMode m_access_mode;
    bool m_side_effects;
};

#endif
