#ifndef PARSER_HPP
#define PARSER_HPP

#include <ace-c/token_stream.hpp>
#include <ace-c/source_location.hpp>
#include <ace-c/compilation_unit.hpp>
#include <ace-c/ast_iterator.hpp>
#include <ace-c/keywords.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/ast/ast_variable_declaration.hpp>
#include <ace-c/ast/ast_function_definition.hpp>
#include <ace-c/ast/ast_function_expression.hpp>
#include <ace-c/ast/ast_array_expression.hpp>
#include <ace-c/ast/ast_type_definition.hpp>
#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_import.hpp>
#include <ace-c/ast/ast_local_import.hpp>
#include <ace-c/ast/ast_integer.hpp>
#include <ace-c/ast/ast_float.hpp>
#include <ace-c/ast/ast_string.hpp>
#include <ace-c/ast/ast_binary_expression.hpp>
#include <ace-c/ast/ast_unary_expression.hpp>
#include <ace-c/ast/ast_function_call.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_member_access.hpp>
#include <ace-c/ast/ast_array_access.hpp>
#include <ace-c/ast/ast_true.hpp>
#include <ace-c/ast/ast_false.hpp>
#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast/ast_block.hpp>
#include <ace-c/ast/ast_if_statement.hpp>
#include <ace-c/ast/ast_while_loop.hpp>
#include <ace-c/ast/ast_print_statement.hpp>
#include <ace-c/ast/ast_try_catch.hpp>
#include <ace-c/ast/ast_type_specification.hpp>
#include <ace-c/ast/ast_type_contract.hpp>
#include <ace-c/ast/ast_return_statement.hpp>

#include <string>

class Parser {
public:
    Parser(AstIterator *ast_iterator, TokenStream *token_stream,
        CompilationUnit *compilation_unit);
    Parser(const Parser &other);

    /** Generate an AST structure from the token stream */
    void Parse(bool expect_module_decl = true);

private:
    AstIterator *m_ast_iterator;
    TokenStream *m_token_stream;
    CompilationUnit *m_compilation_unit;

    Token MatchAhead(Token::TokenType type, int n);
    Token Match(Token::TokenType type, bool read = false);
    Token MatchKeyword(Keywords keyword, bool read = false);
    Token MatchOperator(const Operator *op, bool read = false);
    Token Expect(Token::TokenType type, bool read = false);
    Token ExpectKeyword(Keywords keyword, bool read = false);
    Token ExpectOperator(const Operator *op, bool read = false);
    SourceLocation CurrentLocation() const;

    /** Skip past newlines and semicolons.
        Returns true if any were hit.
    */
    void SkipStatementTerminators();

    int OperatorPrecedence(const Operator *&out);

    std::shared_ptr<AstStatement> ParseStatement();
    std::shared_ptr<AstExpression> ParseTerm();
    std::shared_ptr<AstExpression> ParseParentheses();
    std::shared_ptr<AstInteger> ParseIntegerLiteral();
    std::shared_ptr<AstFloat> ParseFloatLiteral();
    std::shared_ptr<AstString> ParseStringLiteral();
    std::shared_ptr<AstIdentifier> ParseIdentifier();
    std::shared_ptr<AstFunctionCall> ParseFunctionCall();
    std::shared_ptr<AstMemberAccess> ParseMemberAccess(std::shared_ptr<AstExpression> target);
    std::shared_ptr<AstArrayAccess> ParseArrayAccess(std::shared_ptr<AstExpression> target);
    std::shared_ptr<AstTrue> ParseTrue();
    std::shared_ptr<AstFalse> ParseFalse();
    std::shared_ptr<AstNull> ParseNull();
    std::shared_ptr<AstBlock> ParseBlock();
    std::shared_ptr<AstIfStatement> ParseIfStatement();
    std::shared_ptr<AstWhileLoop> ParseWhileLoop();
    std::shared_ptr<AstPrintStatement> ParsePrintStatement();
    std::shared_ptr<AstTryCatch> ParseTryCatchStatement();
    std::shared_ptr<AstExpression> ParseBinaryExpression(int expr_prec,
        std::shared_ptr<AstExpression> left);
    std::shared_ptr<AstExpression> ParseUnaryExpression();
    std::shared_ptr<AstExpression> ParseExpression();
    std::shared_ptr<AstTypeSpecification> ParseTypeSpecification();
    std::shared_ptr<AstTypeContractExpression> ParseTypeContract();
    std::shared_ptr<AstTypeContractExpression> ParseTypeContractExpression();
    std::shared_ptr<AstTypeContractExpression> ParseTypeContractTerm();
    std::shared_ptr<AstTypeContractExpression> ParseTypeContractBinaryExpression(int expr_prec,
        std::shared_ptr<AstTypeContractExpression> left);
    std::shared_ptr<AstVariableDeclaration> ParseVariableDeclaration(bool require_keyword = true);
    std::shared_ptr<AstFunctionDefinition> ParseFunctionDefinition();
    std::shared_ptr<AstFunctionExpression> ParseFunctionExpression(bool func_keyword = true, std::vector<std::shared_ptr<AstParameter>> params = {});
    std::shared_ptr<AstArrayExpression> ParseArrayExpression();
    std::vector<std::shared_ptr<AstParameter>> ParseFunctionParameters();
    std::shared_ptr<AstTypeDefinition> ParseTypeDefinition();
    std::shared_ptr<AstImport> ParseImport();
    std::shared_ptr<AstLocalImport> ParseLocalImport();
    std::shared_ptr<AstReturnStatement> ParseReturnStatement();
};

#endif
