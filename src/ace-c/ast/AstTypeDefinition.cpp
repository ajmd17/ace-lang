#include <ace-c/ast/AstTypeDefinition.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/keywords.hpp>
#include <ace-c/module.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Configuration.hpp>

#include <common/hasher.hpp>
#include <common/my_assert.hpp>

AstTypeDefinition::AstTypeDefinition(const std::string &name,
    const std::vector<std::string> &generic_params,
    const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
    const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_generic_params(generic_params),
      m_members(members),
      m_num_members(0)
{
}

void AstTypeDefinition::Visit(AstVisitor *visitor, Module *mod)
{   
    ASSERT(visitor != nullptr && mod != nullptr);

    if (mod->LookupSymbolType(m_name)) {
        // error; redeclaration of type in module
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redefined_type,
                m_location, m_name));
    } else {
        // open the scope for data members
        mod->m_scopes.Open(Scope());

        // handle generic parameter declarations
        std::vector<SymbolTypePtr_t> generic_param_types;

        const bool is_generic = !m_generic_params.empty();
        for (const std::string &generic_name : m_generic_params) {
            auto it = std::find_if(generic_param_types.begin(), generic_param_types.end(),
                [&generic_name](SymbolTypePtr_t &item) {
                    return item->GetName() == generic_name;
                });

            if (it == generic_param_types.end()) {
                SymbolTypePtr_t type = SymbolType::GenericParameter(generic_name, 
                    nullptr /* substitution is nullptr because this is not a generic instance*/);
                generic_param_types.push_back(type);
                mod->m_scopes.Top().GetIdentifierTable().AddSymbolType(type);
            } else {
                // error; redeclaration of generic parameter
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_generic_parameter_redeclared,
                        m_location, generic_name));
            }
        }

        std::vector<SymbolMember_t> member_types;

        // generate hashes for member names
        std::vector<uint32_t> hashes;
        hashes.reserve(m_members.size());

        for (const auto &mem : m_members) {
            if (mem) {
                mem->Visit(visitor, mod);

                std::string mem_name = mem->GetName();

                if (mem->GetIdentifier()) {
                    SymbolTypePtr_t mem_type = mem->GetIdentifier()->GetSymbolType();

                    // TODO find a better way to set up default assignment for members!
                    // we can't  modify default values of types.
                    //mem_type.SetDefaultValue(mem->GetAssignment());

                    member_types.push_back(std::make_tuple(mem_name, mem_type, mem->GetAssignment()));

                    // generate hash from member name
                    hashes.push_back(hash_fnv_1(mem_name.c_str()));

                    m_num_members++;
                }
            }
        }

        // close the scope for data members
        mod->m_scopes.Close();

        // mangle the type name
        std::string type_name = mod->GetName() + "." + m_name;
        size_t len = type_name.length();

        ASSERT((int)m_num_members < ace::compiler::Config::MAX_DATA_MEMBERS);

        // create static object
        StaticTypeInfo st;
        st.m_size = m_num_members;
        st.m_hashes = hashes;
        st.m_name = new char[len + 1];
        st.m_name[len] = '\0';
        std::strcpy(st.m_name, type_name.c_str());

        int id;

        StaticObject so(st);
        int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
        if (found_id == -1) {
            so.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
            id = so.m_id;
        } else {
            id = found_id;
        }

        delete[] st.m_name;

        SymbolTypePtr_t symbol_type;

        if (!is_generic) {
            symbol_type = SymbolType::Object(m_name, member_types);
        } else {
            symbol_type = SymbolType::Generic(m_name, nullptr, member_types, 
                GenericTypeInfo{ (int)m_generic_params.size(), generic_param_types });
        }

        symbol_type->SetId(id);

        Scope &top_scope = mod->m_scopes.Top();
        top_scope.GetIdentifierTable().AddSymbolType(symbol_type);
    }
}

void AstTypeDefinition::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
}

void AstTypeDefinition::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_type) << " ";
    ss << m_name;
    ss << "{";

    for (const auto &mem : m_members) {
        if (mem) {
            mem->Recreate(ss);
        }
    }

    ss << "}";
}

Pointer<AstStatement> AstTypeDefinition::Clone() const
{
    return CloneImpl();
}
