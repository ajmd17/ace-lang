#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <ace-c/AstVisitor.hpp>
#include <common/my_assert.hpp>

class Compiler : public AstVisitor {
public:
    struct CondInfo {
        AstStatement *cond;
        AstStatement *then_part;
        AstStatement *else_part;
    };

    struct ExprInfo {
        AstExpression *left;
        AstExpression *right;
    };

    /** Compiler a standard if-then-else statement into the program.
        If the `else` expression is nullptr it will be omitted.
    */
    static void CreateConditional(AstVisitor *visitor, Module *mod, CondInfo info);
    /** Standard evaluation order. Load left into register 0,
        then load right into register 1.
        Rinse and repeat.
    */
    static void LoadLeftThenRight(AstVisitor *visitor, Module *mod, ExprInfo info);
    /** Handles the right side before the left side. Used in the case that the
        right hand side is an expression, but the left hand side is just a value.
        If the left hand side is a function call, the right hand side will have to
        be temporarily stored on the stack.
    */
    static void LoadRightThenLeft(AstVisitor *visitor, Module *mod, ExprInfo info);
    /** Loads the left hand side and stores it on the stack.
        Then, the right hand side is loaded into a register,
        and the result is computed.
    */
    static void LoadLeftAndStore(AstVisitor *visitor, Module *mod, ExprInfo info);
    /** Pops from the stack N times. If N is greater than 1,
        the POP_N instruction is generated. Otherwise, the POP
        instruction is generated.
    */
    static void PopStack(AstVisitor *visitor, int amt);

public:
    Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Compiler(const Compiler &other);

    void Compile(bool expect_module_decl = true);

private:
    void CompileInner();
};

#endif