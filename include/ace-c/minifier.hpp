#ifndef MINIFIER_HPP
#define MINIFIER_HPP

#include <ace-c/ast_visitor.hpp>
#include <common/my_assert.hpp>

#include <sstream>

class Minifier : public AstVisitor {
public:
    Minifier(AstIterator *ast_iterator);
    Minifier(const Minifier &other);

    void Minify(std::ostringstream &ss);
};

#endif