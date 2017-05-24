#include <ace-c/ast/AstTypeDefinition.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/emit/NamesPair.hpp>
#include <ace-c/Configuration.hpp>

#include <common/hasher.hpp>
#include <common/my_assert.hpp>

AstTypeDefinition::AstTypeDefinition(const std::string &name,
    const std::vector<std::string> &generic_params,
    const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
    const std::vector<std::shared_ptr<AstEvent>> &events,
    const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_generic_params(generic_params),
      m_members(members),
      m_events(events),
      m_num_members(0)
{
}

void AstTypeDefinition::Visit(AstVisitor *visitor, Module *mod)
{   
    ASSERT(visitor != nullptr && mod != nullptr);

    if (mod->LookupSymbolType(m_name)) {
        // error; redeclaration of type in module
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(
                LEVEL_ERROR,
                Msg_redefined_type,
                m_location,
                m_name
            )
        );
    } else {
        // open the scope for data members
        mod->m_scopes.Open(Scope());

        // handle generic parameter declarations
        std::vector<SymbolTypePtr_t> generic_param_types;

        const bool is_generic = !m_generic_params.empty();

        for (const std::string &generic_name : m_generic_params) {
            auto it = std::find_if(
                generic_param_types.begin(),
                generic_param_types.end(),
                [&generic_name](SymbolTypePtr_t &item) {
                    return item->GetName() == generic_name;
                }
            );

            if (it == generic_param_types.end()) {
                SymbolTypePtr_t type = SymbolType::GenericParameter(
                    generic_name, 
                    nullptr /* substitution is nullptr because this is not a generic instance*/
                );

                generic_param_types.push_back(type);
                mod->m_scopes.Top().GetIdentifierTable().AddSymbolType(type);
            } else {
                // error; redeclaration of generic parameter
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(
                        LEVEL_ERROR,
                        Msg_generic_parameter_redeclared,
                        m_location,
                        generic_name
                    )
                );
            }
        }

        if (!m_events.empty()) {
            std::vector<SymbolMember_t> events_members;
            
            for (const auto &event : m_events) {
                if (event != nullptr) {
                    event->Visit(visitor, mod);
                    events_members.push_back({
                        event->GetKey(),
                        SymbolType::Builtin::FUNCTION,
                        event->GetTrigger()
                    });
                }
            }

            // create a new type for the 'events' field containing all listeners as fields
            m_event_field_type = SymbolType::Object(
                m_name + ".Events",
                events_members
            );
            
            // register the type for 'events' field
            visitor->GetCompilationUnit()->RegisterType(m_event_field_type);

            // builtin members:
            m_members.push_back(std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
                "events",
                // std::shared_ptr<AstTypeSpecification>(new AstTypeSpecification(
                //     SymbolType::Builtin::ARRAY->GetName(),
                //     {
                //         std::shared_ptr<AstTypeSpecification>(new AstTypeSpecification(
                //             SymbolType::Builtin::EVENT->GetName(),
                //             {},
                //             nullptr,
                //             m_location
                //         ))
                //     },
                //     nullptr,
                //     m_location
                // )),
                nullptr,
                m_event_field_type->GetDefaultValue(),
                m_location
            )));
        }

        std::vector<SymbolMember_t> member_types;

        for (const auto &mem : m_members) {
            if (mem != nullptr) {
                mem->Visit(visitor, mod);

                if (mem->GetIdentifier()) {
                    std::string mem_name = mem->GetName();
                    SymbolTypePtr_t mem_type = mem->GetIdentifier()->GetSymbolType();

                    // TODO find a better way to set up default assignment for members!
                    // we can't  modify default values of types.
                    //mem_type.SetDefaultValue(mem->GetAssignment());

                    member_types.push_back(std::make_tuple(
                        mem_name,
                        mem_type,
                        mem->GetAssignment()
                    ));
                }
            }
        }

        // close the scope for data members
        mod->m_scopes.Close();

        SymbolTypePtr_t symbol_type;

        if (!is_generic) {
            symbol_type = SymbolType::Object(
                m_name,
                member_types
            );
        } else {
            symbol_type = SymbolType::Generic(
                m_name,
                nullptr,
                member_types, 
                GenericTypeInfo {
                    (int)m_generic_params.size(),
                    generic_param_types
                }
            );
        }

        // register the main type
        visitor->GetCompilationUnit()->RegisterType(symbol_type);

        // add the type to the identifier table, so it's usable
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
