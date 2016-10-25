#include <athens/ast/ast_type_definition.hpp>
#include <athens/ast/ast_null.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

AstTypeDefinition::AstTypeDefinition(const std::string &name,
    const std::vector<std::shared_ptr<AstDeclaration>> &members,
    const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_members(members)
{
}

void AstTypeDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    ObjectType object_type(m_name, std::shared_ptr<AstNull>(new AstNull(SourceLocation::eof)));

    if (mod->LookUpUserType(m_name, object_type)) {
        // error; redeclaration of type in module
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redefined_type,
                m_location, m_name));
    } else {
        // add all members
        for (const auto &mem : m_members) {
            if (mem != nullptr) {
                if (!object_type.HasDataMember(mem->GetName())) {
                    DataMember_t dm(mem->GetName(), mem->GetObjectType());
                    object_type.AddDataMember(dm);
                } else {
                    // error; redeclaration of data member
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_redeclared_data_member,
                            mem->GetLocation(), mem->GetName(), m_name));
                }
            }
        }

        mod->AddUserType(object_type);
    }
}

void AstTypeDefinition::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
}
