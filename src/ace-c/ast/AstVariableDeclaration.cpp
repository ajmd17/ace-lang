#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/ast/AstTypeObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>
#include <ace-c/SemanticAnalyzer.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <iostream>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstPrototypeSpecification> &proto,
    //const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstExpression> &assignment,
    const std::vector<std::shared_ptr<AstParameter>> &template_params,
    bool is_const,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_proto(proto),
      m_assignment(assignment),
      m_template_params(template_params),
      m_is_const(is_const),
      m_assignment_already_visited(false)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    SymbolTypePtr_t symbol_type;
    std::vector<GenericInstanceTypeInfo::Arg> ident_template_params;

    if (!m_template_params.empty()) {
        // declare template params in a new scope
        // open the new scope for parameters
        mod->m_scopes.Open(Scope(SCOPE_TYPE_NORMAL, 0));

        for (auto &param : m_template_params) {
            ASSERT(param != nullptr);
            param->Visit(visitor, mod);

            ASSERT(param->GetIdentifier() != nullptr);

            ident_template_params.push_back(GenericInstanceTypeInfo::Arg {
                param->GetName(),
                param->GetIdentifier()->GetSymbolType(),
                param->GetDefaultValue()
            });
        }
    }

    if (m_proto == nullptr && m_assignment == nullptr) {
        // error; requires either type, or assignment.
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_missing_type_and_assignment,
            m_location,
            m_name
        ));
    } else {
        if (m_assignment != nullptr) {
            m_real_assignment = m_assignment;
        }

        // the type_strict flag means that errors will be shown if
        // the assignment type and the user-supplied type differ.
        // it is to be turned off for built-in (default) values
        // for example, the default type of Array is Any.
        // this flag will allow an Array(Float) to be constructed 
        // with an empty array of type Array(Any)
        bool is_type_strict = true;

        if (m_proto != nullptr) {
            m_proto->Visit(visitor, mod);

            ASSERT(m_proto->GetHeldType() != nullptr);
            symbol_type = m_proto->GetHeldType();

            std::shared_ptr<AstExpression> default_value = m_proto->GetDefaultValue();

            /*ASSERT(m_proto->GetExprType() != nullptr);
            m_constructor_type = m_proto->GetExprType();

            const bool is_type = m_constructor_type == BuiltinTypes::TYPE_TYPE;

            m_instance_type = BuiltinTypes::ANY;

            if (const AstIdentifier *as_ident = dynamic_cast<AstIdentifier*>(m_proto.get())) {
                if (const auto current_value = as_ident->GetProperties().GetIdentifier()->GetCurrentValue()) {
                    if (AstTypeObject *type_object = dynamic_cast<AstTypeObject*>(current_value.get())) {
                        ASSERT(type_object->GetHeldType() != nullptr);
                        m_instance_type = type_object->GetHeldType();

                        SymbolMember_t proto_member;
                        if (type_object->GetHeldType()->FindMember("$proto", proto_member)) {
                            m_object_value = std::get<2>(proto_member); // NOTE: may be null causing NEW operand to be emitted
                        }
                    } else if (!is_type) {
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_not_a_type,
                            m_location
                        ));
                    }
                }
            } else if (!is_type) {
                visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                    LEVEL_ERROR,
                    Msg_not_a_type,
                    m_location
                ));
            }*/

            //symbol_type = m_type_specification->GetSpecifiedType();
            //ASSERT(symbol_type != nullptr);

            if (symbol_type == BuiltinTypes::ANY) {
                // Any type is reserved for method parameters
                visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                    LEVEL_ERROR,
                    Msg_any_reserved_for_parameters,
                    m_location
                ));
            }

            // if no assignment provided, set the assignment to be the default value of the provided type
            if (m_real_assignment == nullptr) {
                // generic/non-concrete types that have default values
                // will get assigned to their default value without causing
                // an error
                if (default_value != nullptr) {
                    // Assign variable to the default value for the specified type.
                    m_real_assignment = CloneAstNode(default_value);
                    // built-in assignment, turn off strict mode
                    is_type_strict = false;
                } else if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                    // generic not yet promoted to an instance.
                    // since there is no assignment to go by, we can't promote it

                    visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                        LEVEL_ERROR,
                        Msg_generic_parameters_missing,
                        m_location,
                        symbol_type->GetName(),
                        symbol_type->GetGenericInfo().m_num_parameters
                    ));
                } else {
                    // generic parameters will be resolved upon instantiation
                    if (!symbol_type->IsGenericParameter()) {
                        // no default assignment for this type
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_type_no_default_assignment,
                            m_location,
                            symbol_type->GetName()
                        ));
                    }
                }
            }
        }

        if (m_real_assignment == nullptr) {
            m_real_assignment.reset(new AstUndefined(m_location));
        }

        if (!m_assignment_already_visited) {
            // visit assignment
            m_real_assignment->Visit(visitor, mod);
        }

        if (m_assignment != nullptr) {
            // has received an explicit assignment
            // make sure type is compatible with assignment
            SymbolTypePtr_t assignment_type = m_real_assignment->GetExprType();
            ASSERT(assignment_type != nullptr);

            if (m_proto != nullptr) {
                // symbol_type should be the user-specified type
                symbol_type = SymbolType::GenericPromotion(symbol_type, assignment_type);
                ASSERT(symbol_type != nullptr);

                // generic not yet promoted to an instance
                if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                        LEVEL_ERROR,
                        Msg_generic_parameters_missing,
                        m_location,
                        symbol_type->GetName(),
                        symbol_type->GetGenericInfo().m_num_parameters
                    ));
                }

                if (is_type_strict) {
                    SymbolTypePtr_t comparison_type = symbol_type;

                    // unboxing values
                    // note that this is below the default assignment check,
                    // because these "box" types may have a default assignment of their own
                    // (or they may intentionally not have one)
                    // e.g Maybe(T) defaults to null, and Const(T) has no assignment.
                    if (symbol_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                        if (symbol_type->IsBoxedType()) {
                            comparison_type = symbol_type->GetGenericInstanceInfo().m_generic_args[0].m_type;
                            ASSERT(comparison_type != nullptr);
                        }
                    }

                    SemanticAnalyzer::Helpers::EnsureTypeAssignmentCompatibility(
                        visitor,
                        mod,
                        comparison_type,
                        assignment_type,
                        m_real_assignment->GetLocation()
                    );
                }
            } else {
                // Set the type to be the deduced type from the expression.
                symbol_type = assignment_type;
            }
        }
    }

    if (!m_template_params.empty()) {
        // close template param scope
        mod->m_scopes.Close();
    }

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier != nullptr) {
        if (m_is_const) {
            m_identifier->SetFlags(m_identifier->GetFlags() | IdentifierFlags::FLAG_CONST);
        }

        if (!m_template_params.empty()) {
            m_identifier->SetTemplateParams(ident_template_params);
            m_identifier->SetFlags(m_identifier->GetFlags() | IdentifierFlags::FLAG_GENERIC);
        }

        m_identifier->SetSymbolType(symbol_type);
        m_identifier->SetCurrentValue(m_real_assignment);
    }
}

std::unique_ptr<Buildable> AstVariableDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    if (!m_template_params.empty()) {
        // generics do not build anything
        return nullptr;
    }

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    ASSERT(m_real_assignment != nullptr);

    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
        // get current stack size
        const int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        chunk->Append(m_real_assignment->Build(visitor, mod));

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
    } else {
        // if assignment has side effects but variable is unused,
        // compile the assignment in anyway.
        if (m_real_assignment->MayHaveSideEffects()) {
            chunk->Append(m_real_assignment->Build(visitor, mod));
        }
    }

    return std::move(chunk);
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_real_assignment != nullptr) {
        m_real_assignment->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstVariableDeclaration::Clone() const
{
    return CloneImpl();
}
