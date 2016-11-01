#include <ace-c/ast/ast_type_definition.hpp>
#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast/ast_object.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/emit/static_object.hpp>

AstTypeDefinition::AstTypeDefinition(const std::string &name,
    const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
    const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_members(members),
      m_num_members(0)
{
}

void AstTypeDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    ObjectType object_type(m_name, nullptr);

    if (mod->LookUpUserType(m_name, object_type)) {
        // error; redeclaration of type in module
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redefined_type,
                m_location, m_name));
    } else {
        // open the scope for data members
        mod->m_scopes.Open(Scope());

        for (const auto &mem : m_members) {
            if (mem != nullptr) {
                mem->Visit(visitor, mod);

                if (!object_type.HasDataMember(mem->GetName())) {
                    ObjectType mem_type = mem->GetObjectType();
                    mem_type.SetDefaultValue(mem->GetAssignment());

                    DataMember_t dm(mem->GetName(), mem_type);
                    object_type.AddDataMember(dm);
                    m_num_members++;
                }
            }
        }

        // close the scope for data members
        mod->m_scopes.Close();

        // mangle the type name
        std::string type_name = mod->GetName() + "." + m_name;
        size_t len = type_name.length();

        // create static object
        StaticTypeInfo st;
        st.m_size = m_num_members;
        st.m_name = new char[len + 1];
        st.m_name[len] = '\0';
        std::strcpy(st.m_name, type_name.c_str());

        StaticObject so(st);
        int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
        if (found_id == -1) {
            so.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
            object_type.SetStaticId(so.m_id);
        } else {
            object_type.SetStaticId(found_id);
        }

        delete[] st.m_name;

        object_type.SetDefaultValue(std::shared_ptr<AstObject>(new AstObject(object_type, SourceLocation::eof)));
        mod->AddUserType(object_type);
    }
}

void AstTypeDefinition::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
}
