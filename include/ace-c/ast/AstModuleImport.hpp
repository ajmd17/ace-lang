#ifndef AST_MODULE_IMPORT_HPP
#define AST_MODULE_IMPORT_HPP

#include <ace-c/ast/AstImport.hpp>

#include <string>

class AstModuleImport : public AstImport {
public:
    AstModuleImport(
      const std::shared_ptr<AstModuleAccess> &mod_access,
      const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::shared_ptr<AstModuleAccess> m_mod_access;

    inline Pointer<AstModuleImport> CloneImpl() const
    {
        return Pointer<AstModuleImport>(new AstModuleImport(
            CloneAstNode(m_mod_access),
            m_location
        ));
    }
};

#endif
