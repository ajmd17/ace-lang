#include <ace-c/ast/AstPrototypeSpecification.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/ast/AstHasExpression.hpp>
#include <ace-c/ast/AstIdentifier.hpp>
#include <ace-c/ast/AstTypeObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Compiler.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>

AstPrototypeSpecification::AstPrototypeSpecification(
    const std::shared_ptr<AstExpression> &proto,
    const SourceLocation &location)
    : AstStatement(location),
      m_proto(proto)
{
}

void AstPrototypeSpecification::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_proto != nullptr);
    m_proto->Visit(visitor, mod);

    ASSERT(m_proto->GetExprType() != nullptr);
    SymbolTypePtr_t constructor_type = m_proto->GetExprType();

    std::cout << "constructor_type = " << constructor_type->GetName() << "\n";

    const bool is_type = constructor_type == BuiltinTypes::TYPE_TYPE;

    m_symbol_type = BuiltinTypes::ANY;
    m_prototype_type = BuiltinTypes::ANY;

    if (constructor_type != BuiltinTypes::ANY) {
        if (const AstIdentifier *as_ident = dynamic_cast<AstIdentifier*>(m_proto.get())) {
            if (const auto current_value = as_ident->GetProperties().GetIdentifier()->GetCurrentValue()) {
                if (AstTypeObject *type_object = dynamic_cast<AstTypeObject*>(current_value.get())) {
                    ASSERT(type_object->GetHeldType() != nullptr);
                    m_symbol_type = type_object->GetHeldType();

                    SymbolMember_t proto_member;
                    if (type_object->GetHeldType()->FindMember("$proto", proto_member)) {
                        m_prototype_type = std::get<1>(proto_member);
                        m_default_value = std::get<2>(proto_member);
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
        }
    }
}

std::unique_ptr<Buildable> AstPrototypeSpecification::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_proto != nullptr);
    return m_proto->Build(visitor, mod);
}

void AstPrototypeSpecification::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_proto != nullptr);
    m_proto->Optimize(visitor, mod);
}

Pointer<AstStatement> AstPrototypeSpecification::Clone() const
{
    return CloneImpl();
}
