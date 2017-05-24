#ifndef AST_MODULE_IMPORT_HPP
#define AST_MODULE_IMPORT_HPP

#include <ace-c/ast/AstImport.hpp>

#include <string>

class AstModuleImport : public AstImport {
public:
    AstModuleImport(
      const std::string &mod_name,
      const std::shared_ptr<AstModuleImport> &right,
      const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::string m_mod_name;
    std::shared_ptr<AstModuleImport> m_right;

    inline Pointer<AstModuleImport> CloneImpl() const
    {
        return Pointer<AstModuleImport>(new AstModuleImport(
            m_mod_name,
            CloneAstNode(m_right),
            m_location
        ));
    }
};

#endif
