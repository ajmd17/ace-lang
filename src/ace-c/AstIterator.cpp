#include <ace-c/AstIterator.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstTypeObject.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

AstIterator::AstIterator()
    : m_position(0)
{

    // m_list = std::vector<std::shared_ptr<AstStatement>> {
    //    std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
    //         "Int",
    //         nullptr,
    //         std::shared_ptr<AstTypeObject>(new AstTypeObject(
    //             BuiltinTypes::INT, nullptr, SourceLocation::eof
    //         )),
    //         false,
    //         SourceLocation::eof
    //     ))
    // };
}

AstIterator::AstIterator(const AstIterator &other)
    : m_position(other.m_position),
      m_list(other.m_list)
{
}
