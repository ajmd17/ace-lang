#include <ace-c/ast/AstModuleProperty.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/hasher.hpp>

#include <iostream>

AstModuleProperty::AstModuleProperty(
    const std::string &field_name,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_field_name(field_name),
      m_expr_type(BuiltinTypes::UNDEFINED)
{
}

void AstModuleProperty::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    if (m_field_name == "name") {
        m_expr_value = std::shared_ptr<AstString>(new AstString(
            mod->GetName(),
            m_location
        ));
    } else if (m_field_name == "path") {
        m_expr_value = std::shared_ptr<AstString>(new AstString(
            mod->GetLocation().GetFileName(),
            m_location
        ));
    }
    
    if (m_expr_value != nullptr) {
      m_expr_value->Visit(visitor, mod);

      ASSERT(m_expr_value->GetSymbolType() != nullptr);
      m_expr_type = m_expr_value->GetSymbolType();
    } else {
      visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
          LEVEL_ERROR,
          Msg_not_a_data_member,
          m_location,
          m_field_name,
          BuiltinTypes::MODULE_INFO->GetName()
      ));
    }
}

std::unique_ptr<Buildable> AstModuleProperty::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_expr_value != nullptr);
    return m_expr_value->Build(visitor, mod);
}

void AstModuleProperty::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr_value != nullptr);
    m_expr_value->Optimize(visitor, mod);
}

Pointer<AstStatement> AstModuleProperty::Clone() const
{
    return CloneImpl();
}

Tribool AstModuleProperty::IsTrue() const
{
    ASSERT(m_expr_value != nullptr);
    return m_expr_value->IsTrue();
}

bool AstModuleProperty::MayHaveSideEffects() const
{
    if (m_expr_value != nullptr) {
        return m_expr_value->MayHaveSideEffects();
    } else {
        return false;
    }
}

SymbolTypePtr_t AstModuleProperty::GetSymbolType() const
{
    ASSERT(m_expr_type != nullptr);
    return m_expr_type;
}
