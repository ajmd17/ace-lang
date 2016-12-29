#ifndef AST_STATEMENT_HPP
#define AST_STATEMENT_HPP

#include <ace-c/SourceLocation.hpp>

#include <memory>
#include <sstream>

// Forward declarations
class AstVisitor;
class Module;

class AstStatement {
    friend class AstIterator;
public:
    AstStatement(const SourceLocation &location);
    virtual ~AstStatement() = default;

    inline SourceLocation &GetLocation() { return m_location; }
    inline const SourceLocation &GetLocation() const { return m_location; }

    virtual void Visit(AstVisitor *visitor, Module *mod) = 0;
    virtual void Build(AstVisitor *visitor, Module *mod) = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) = 0;
    virtual void Recreate(std::ostringstream &ss) = 0;

protected:
    SourceLocation m_location;
};

#endif
