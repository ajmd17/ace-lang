#include <ace-c/ast/AstEvent.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <iostream>

AstConstantEvent::AstConstantEvent(const std::shared_ptr<AstConstant> &key,
    const std::shared_ptr<AstFunctionExpression> &trigger,
    const SourceLocation &location)
    : AstEvent(trigger, location),
      m_key(key)
{
}

std::shared_ptr<AstExpression> AstConstantEvent::GetKey() const
{
    return m_key;
}

std::string AstConstantEvent::GetKeyName() const
{
    ASSERT(m_key != nullptr);

    if (AstString *as_string = dynamic_cast<AstString*>(m_key.get())) {
        return as_string->GetValue();
    } else if (AstFloat *as_float = dynamic_cast<AstFloat*>(m_key.get())) {
        return std::to_string(as_float->FloatValue());
    } else {
        return std::to_string(m_key->IntValue());
    }
}

void AstConstantEvent::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_key != nullptr);
    m_key->Visit(visitor, mod);

    AstEvent::Visit(visitor, mod);
}

std::unique_ptr<Buildable> AstConstantEvent::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    return AstEvent::Build(visitor, mod);
}

void AstConstantEvent::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_key != nullptr);
    m_key->Optimize(visitor, mod);

    AstEvent::Optimize(visitor, mod);
}

void AstConstantEvent::Recreate(std::ostringstream &ss)
{
}

Pointer<AstStatement> AstConstantEvent::Clone() const
{
    return CloneImpl();
}


AstEvent::AstEvent(const std::shared_ptr<AstFunctionExpression> &trigger,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
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

std::unique_ptr<Buildable> AstEvent::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_trigger != nullptr);
    return m_trigger->Build(visitor, mod);
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
