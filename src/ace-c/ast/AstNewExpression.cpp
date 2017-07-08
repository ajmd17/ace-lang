#include <ace-c/ast/AstNewExpression.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstNewExpression::AstNewExpression(
    const std::shared_ptr<AstTypeSpecification> &type_expr,
    const std::shared_ptr<AstArgumentList> &arg_list,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_type_expr(type_expr),
      m_arg_list(arg_list)
{
}

void AstNewExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_type_expr != nullptr);
    m_type_expr->Visit(visitor, mod);

    SymbolTypePtr_t object_type = m_type_expr->GetSymbolType();
    ASSERT(object_type != nullptr);

    // get default value
    if (auto object_value = object_type->GetDefaultValue()) {
        bool should_call_constructor = false;

        if (object_type != BuiltinTypes::ANY) {
            const bool has_written_constructor = object_type->FindMember("new") != nullptr;
            const bool has_args = m_arg_list != nullptr && !m_arg_list->GetArguments().empty();

            if (has_written_constructor || has_args) {
                should_call_constructor = true;
            }
        }

        if (should_call_constructor) {
            std::vector<std::shared_ptr<AstArgument>> args;

            if (m_arg_list != nullptr) {
                args = m_arg_list->GetArguments();
            }

            m_constructor_call.reset(new AstCallExpression(
                std::shared_ptr<AstMember>(new AstMember(
                    "new",
                    object_value,
                    m_location
                )),
                args,
                true,
                m_location
            ));

            m_constructor_call->Visit(visitor, mod);
        }

        m_object_value = object_value;
    } else {
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_type_no_default_assignment,
            m_location,
            object_type->GetName()
        ));
    }
}

std::unique_ptr<Buildable> AstNewExpression::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    ASSERT(m_type_expr != nullptr);
    chunk->Append(m_type_expr->Build(visitor, mod));

    if (m_constructor_call != nullptr) {
        chunk->Append(m_constructor_call->Build(visitor, mod));
    } else {
        // build in the value
        ASSERT(m_object_value != nullptr);
        chunk->Append(m_object_value->Build(visitor, mod));
    }

    return std::move(chunk);
}

void AstNewExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_type_expr != nullptr);
    m_type_expr->Optimize(visitor, mod);

    if (m_constructor_call != nullptr) {
        m_constructor_call->Optimize(visitor, mod);
    } else {
        // build in the value
        ASSERT(m_object_value != nullptr);
        m_object_value->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstNewExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstNewExpression::IsTrue() const
{
    ASSERT(m_object_value != nullptr);
    return m_object_value->IsTrue();
}

bool AstNewExpression::MayHaveSideEffects() const
{
    return true;
}

SymbolTypePtr_t AstNewExpression::GetSymbolType() const
{
    ASSERT(m_type_expr != nullptr);
    ASSERT(m_type_expr->GetSymbolType() != nullptr);

    return m_type_expr->GetSymbolType();
}
