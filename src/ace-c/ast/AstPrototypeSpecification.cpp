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

    m_symbol_type = BuiltinTypes::ANY; // defaults to dynamic type.
    m_prototype_type = BuiltinTypes::ANY;

    if (constructor_type != BuiltinTypes::ANY) {
        const bool is_type = constructor_type == BuiltinTypes::TYPE_TYPE;

        if (is_type) {
            const AstExpression *value_of = m_proto->GetValueOf();

            // FIXME: this dynamic_casting stuff is nasty and we need a better way other than GetValueOf()
            // and such. having 2 separate ways of getting expression types (AstTypeObject vs. the internal SymbolType)
            if (const AstTypeObject *type_obj = dynamic_cast<const AstTypeObject*>(value_of)) {
                ASSERT(type_obj->GetHeldType() != nullptr);
                m_symbol_type = type_obj->GetHeldType();

                SymbolMember_t proto_member;
                if (m_symbol_type->FindMember("$proto", proto_member)) {
                    m_prototype_type = std::get<1>(proto_member);

                    // only give default value IFF it is a built-in type
                    // this is to prevent huge prototypes being inlined.
                    if (m_prototype_type->GetTypeClass() == TYPE_BUILTIN) {
                        m_default_value = std::get<2>(proto_member);
                    }
                }
            }
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_not_a_type,
                m_location,
                constructor_type->GetName()
            ));
        }
    } // if not, it is a dynamic type
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
