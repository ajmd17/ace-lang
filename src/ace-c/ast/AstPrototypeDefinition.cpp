#include <ace-c/ast/AstPrototypeDefinition.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstObject.hpp>
#include <ace-c/ast/AstTypeObject.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/hasher.hpp>
#include <common/my_assert.hpp>

AstPrototypeDefinition::AstPrototypeDefinition(const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &base_specification,
    const std::vector<std::string> &generic_params,
    const std::vector<std::shared_ptr<AstVariableDeclaration>> &members,
    const std::vector<std::shared_ptr<AstVariableDeclaration>> &static_members,
    const std::vector<std::shared_ptr<AstEvent>> &events,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_base_specification(base_specification),
      m_generic_params(generic_params),
      m_members(members),
      m_static_members(static_members),
      m_events(events),
      m_num_members(0)
{
}

void AstPrototypeDefinition::Visit(AstVisitor *visitor, Module *mod)
{   
    ASSERT(visitor != nullptr && mod != nullptr);

    // open the scope for data members
    mod->m_scopes.Open(Scope(SCOPE_TYPE_TYPE_DEFINITION, 0));

    // handle generic parameter declarations
    std::vector<SymbolTypePtr_t> generic_param_types;

    const bool is_generic = !m_generic_params.empty();

    for (const std::string &generic_name : m_generic_params) {
        // add an identifier with the type: Type for each generic param.
        // in the future, allow users to explicitly provide types for generic params
        if (mod->LookUpIdentifier(generic_name, true) != nullptr) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_generic_parameter_redeclared,
                m_location,
                generic_name
            ));
        } else {
            SymbolTypePtr_t generic_param_type = SymbolType::GenericParameter(
                generic_name, 
                nullptr // substitution is nullptr because this is not a generic instance
            );

            generic_param_type->SetDefaultValue(std::shared_ptr<AstTypeObject>(new AstTypeObject(
                generic_param_type,
                nullptr,
                m_location
            )));

            generic_param_types.push_back(generic_param_type);

            Identifier *generic_param_ident = mod->m_scopes.Top().GetIdentifierTable().AddIdentifier(generic_name);
            ASSERT(generic_param_ident != nullptr);

            generic_param_ident->SetSymbolType(BuiltinTypes::TYPE_TYPE);
            generic_param_ident->SetCurrentValue(generic_param_type->GetDefaultValue());
        }

        /*auto it = std::find_if(
            generic_param_types.begin(),
            generic_param_types.end(),
            [&generic_name](SymbolTypePtr_t &item) {
                return item->GetName() == generic_name;
            }
        );

        if (it == generic_param_types.end()) {
            SymbolTypePtr_t type = SymbolType::GenericParameter(
                generic_name, 
                nullptr // substitution is nullptr because this is not a generic instance
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
        }*/
    }

    if (!m_events.empty()) {
        std::vector<std::shared_ptr<AstExpression>> event_items;
        // each event item is an array of size 2 (could be tuple in the future?)
        for (size_t i = 0; i < m_events.size(); i++) {
            const std::shared_ptr<AstEvent> &event = m_events[i];
            if (event != nullptr) {
                //event->Visit(visitor, mod);
                event_items.push_back(std::shared_ptr<AstArrayExpression>(new AstArrayExpression(
                    { event->GetKey(), event->GetTrigger() },
                    m_location
                )));
            }
        }

        // builtin members:
        m_members.push_back(std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
            "$events",
            nullptr,
            std::shared_ptr<AstArrayExpression>(new AstArrayExpression(
                event_items,
                m_location
            )),
            {},
            false,
            m_location
        )));
    }

    std::vector<SymbolMember_t> member_types;

    for (const auto &mem : m_members) {
        if (mem != nullptr) {
            mem->Visit(visitor, mod);

            ASSERT(mem->GetIdentifier() != nullptr);

            std::string mem_name = mem->GetName();
            SymbolTypePtr_t mem_type = mem->GetIdentifier()->GetSymbolType();
            
            member_types.push_back(SymbolMember_t {
                mem_name,
                mem_type,
                mem->GetRealAssignment()
            });
        }
    }

    // close the scope for data members
    mod->m_scopes.Close();

    SymbolTypePtr_t prototype_type = SymbolType::Object(
        m_name + "Instance", // Prototype type
        member_types,
        BuiltinTypes::OBJECT
    );

    // TODO: allow custom bases (which would have to extend Type somewhere)
    SymbolTypePtr_t base_type = BuiltinTypes::TYPE_TYPE;

    std::vector<SymbolMember_t> static_members;

    // check if one with the name $proto already exists.
    bool proto_found = false;
    bool base_found = false;

    for (const auto &mem : m_static_members) {
        ASSERT(mem != nullptr);

        if (mem->GetName() == "$proto") {
            proto_found = true;
        } else if (mem->GetName() == "base") {
            base_found = true;
        }

        if (proto_found && base_found) {
            break; // no need to keep searching
        }
    }

    if (!proto_found) { // no custom '$proto' member, add default.
        static_members.push_back(SymbolMember_t {
            "$proto",
            prototype_type,
            std::shared_ptr<AstTypeObject>(new AstTypeObject(
                prototype_type,
                nullptr,
                m_location
            ))
        });
    }

    if (!base_found) { // no custom 'base' member, add default
        static_members.push_back(SymbolMember_t {
            "base",
            base_type,
            std::shared_ptr<AstTypeObject>(new AstTypeObject(
                base_type,
                nullptr,
                m_location
            ))
        });
    }


    // open the scope for static data members
    mod->m_scopes.Open(Scope(SCOPE_TYPE_TYPE_DEFINITION, 0));

    for (const auto &mem : m_static_members) {
        ASSERT(mem != nullptr);
        mem->Visit(visitor, mod);

        std::string mem_name = mem->GetName();

        ASSERT(mem->GetIdentifier() != nullptr);
        SymbolTypePtr_t mem_type = mem->GetIdentifier()->GetSymbolType();
        
        static_members.push_back(SymbolMember_t {
            mem_name,
            mem_type,
            mem->GetRealAssignment()
        });
    }

    // close the scope for static data members
    mod->m_scopes.Close();

    if (!is_generic) {
        m_symbol_type = SymbolType::Extend(
            m_name,
            base_type,
            static_members
        );
    } else {
        m_symbol_type = SymbolType::Generic(
            m_name,
            nullptr,
            static_members, 
            GenericTypeInfo {
                (int)m_generic_params.size(),
                generic_param_types
            },
            base_type
        );

        m_symbol_type->SetDefaultValue(std::shared_ptr<AstObject>(
            new AstObject(m_symbol_type, SourceLocation::eof)
        ));
    }

    AstDeclaration::Visit(visitor, mod);

    ASSERT(m_identifier != nullptr);
    
    m_identifier->SetSymbolType(base_type);
    m_identifier->SetCurrentValue(std::shared_ptr<AstTypeObject>(new AstTypeObject(
        m_symbol_type,
        nullptr, // prototype - TODO
        m_location
    )));
}

std::unique_ptr<Buildable> AstPrototypeDefinition::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        const int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        ASSERT(m_identifier->GetCurrentValue() != nullptr);
        chunk->Append(m_identifier->GetCurrentValue()->Build(visitor, mod));

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        { // add instruction to store on stack
            auto instr_push = BytecodeUtil::Make<RawOperation<>>();
            instr_push->opcode = PUSH;
            instr_push->Accept<uint8_t>(rp);
            chunk->Append(std::move(instr_push));
        }

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }

    return std::move(chunk);
}

void AstPrototypeDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstPrototypeDefinition::Clone() const
{
    return CloneImpl();
}
