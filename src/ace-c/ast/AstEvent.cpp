#include <ace-c/ast/AstEvent.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <iostream>

AstEvent::AstEvent(const std::string &key,
    const std::shared_ptr<AstFunctionExpression> &trigger,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_key(key),
      m_trigger(trigger)
{
}

void AstEvent::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_trigger != nullptr);
    m_trigger->Visit(visitor, mod);
}

void AstEvent::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_trigger != nullptr);
    m_trigger->Build(visitor, mod);
}

void AstEvent::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_trigger != nullptr);
    m_trigger->Optimize(visitor, mod);
}

void AstEvent::Recreate(std::ostringstream &ss)
{
}

Pointer<AstStatement> AstEvent::Clone() const
{
    return CloneImpl();
}

int AstEvent::IsTrue() const
{
    return 1;
}

bool AstEvent::MayHaveSideEffects() const
{
    return false;
}

SymbolTypePtr_t AstEvent::GetSymbolType() const
{
    return SymbolType::Builtin::EVENT;
}
