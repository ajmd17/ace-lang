#include <ace-c/parser.hpp>

#include <memory>
#include <cstdlib>
#include <cstdio>
#include <iostream>

Parser::Parser(AstIterator *ast_iterator, TokenStream *token_stream,
        CompilationUnit *compilation_unit)
    : m_ast_iterator(ast_iterator),
      m_token_stream(token_stream),
      m_compilation_unit(compilation_unit)
{
}

Parser::Parser(const Parser &other)
    : m_ast_iterator(other.m_ast_iterator),
      m_token_stream(other.m_token_stream),
      m_compilation_unit(other.m_compilation_unit)
{
}

Token Parser::MatchAhead(Token::TokenType type, int n)
{
    const Token peek = m_token_stream->Peek(n);
    
    if (peek && peek.GetType() == type) {
        return peek;
    }
    
    return Token::EMPTY;
}

Token Parser::Match(Token::TokenType type, bool read)
{
    const Token peek = m_token_stream->Peek();
    
    if (peek && peek.GetType() == type) {
        if (read && m_token_stream->HasNext()) {
            m_token_stream->Next();
        }
        
        return peek;
    }
    
    return Token::EMPTY;
}

Token Parser::MatchKeyword(Keywords keyword, bool read)
{
    const Token peek = m_token_stream->Peek();
    
    if (peek && peek.GetType() == Token::TokenType::Token_keyword) {
        std::string str = Keyword::ToString(keyword);
        if (peek.GetValue() == str) {
            if (read && m_token_stream->HasNext()) {
                m_token_stream->Next();
            }
            
            return peek;
        }
    }
    
    return Token::EMPTY;
}

Token Parser::MatchOperator(const Operator *op, bool read)
{
    const Token peek = m_token_stream->Peek();
    
    if (peek && peek.GetType() == Token::TokenType::Token_operator) {
        std::string str = op->ToString();
        if (peek.GetValue() == str) {
            if (read && m_token_stream->HasNext()) {
                m_token_stream->Next();
            }
            return peek;
        }
    }
    
    return Token::EMPTY;
}

Token Parser::Expect(Token::TokenType type, bool read)
{
    const Token token = Match(type, read);
    
    if (!token) {
        SourceLocation location = CurrentLocation();

        ErrorMessage error_msg;
        std::string error_str;

        switch (type) {
        case Token::TokenType::Token_identifier:
            error_msg = Msg_expected_identifier;
            break;
        default:
            error_msg = Msg_expected_token;
            error_str = Token::TokenTypeToString(type);
        }

        CompilerError error(Level_fatal,
            error_msg, location, error_str);

        m_compilation_unit->GetErrorList().AddError(error);
    }

    return token;
}

Token Parser::ExpectKeyword(Keywords keyword, bool read)
{
    const Token token = MatchKeyword(keyword, read);
    
    if (!token) {
        SourceLocation location = CurrentLocation();
        if (read && m_token_stream->HasNext()) {
            m_token_stream->Next();
        }

        ErrorMessage error_msg;
        std::string error_str;

        switch (keyword) {
        case Keyword_module:
            error_msg = Msg_expected_module;
            break;
        default:
            error_msg = Msg_expected_token;
            error_str = Keyword::ToString(keyword);
        }

        CompilerError error(Level_fatal,
            error_msg, location, error_str);

        m_compilation_unit->GetErrorList().AddError(error);
    }

    return token;
}

Token Parser::ExpectOperator(const Operator *op, bool read)
{
    const Token token = MatchOperator(op, read);
    if (!token) {
        SourceLocation location = CurrentLocation();
        if (read && m_token_stream->HasNext()) {
            m_token_stream->Next();
        }

        ErrorMessage error_msg = Msg_expected_token;
        std::string error_str = op->ToString();

        CompilerError error(Level_fatal,
            error_msg, location, error_str);

        m_compilation_unit->GetErrorList().AddError(error);
    }

    return token;
}

SourceLocation Parser::CurrentLocation() const
{
    if (m_token_stream->GetSize() != 0 && !m_token_stream->HasNext()) {
        return m_token_stream->Last().GetLocation();
    }

    return m_token_stream->Peek().GetLocation();
}

void Parser::SkipStatementTerminators()
{
    // read past statement terminator tokens
    while (Match(Token::TokenType::Token_semicolon, true) ||
           Match(Token::TokenType::Token_newline, true));
}

void Parser::Parse(bool expect_module_decl)
{
    if (expect_module_decl) {
        if (std::shared_ptr<AstModuleDeclaration> module_ast = ParseModuleDeclaration()) {
            m_ast_iterator->Push(module_ast);
            
            while (m_token_stream->HasNext()) {
                if (Match(Token::TokenType::Token_semicolon, true) ||
                    Match(Token::TokenType::Token_newline, true)) {
                    continue;
                }

                // check for modules declared after the first
                if (MatchKeyword(Keyword_module, false)) {
                    m_ast_iterator->Push(ParseModuleDeclaration());
                } else {
                    SourceLocation location = CurrentLocation();

                    // call ParseStatement() to read to next
                    ParseStatement();

                    // error, statement outside of module
                    CompilerError error(Level_fatal, Msg_statement_outside_module, location);
                    m_compilation_unit->GetErrorList().AddError(error);
                }
            }
        }
    } else {
        while (m_token_stream->HasNext()) {
            SourceLocation location = CurrentLocation();

            // skip statement terminator tokens
            if (Match(Token::TokenType::Token_semicolon, true) ||
                Match(Token::TokenType::Token_newline, true)) {
                continue;
            }

            std::shared_ptr<AstStatement> stmt;

            // check for module declaration
            if (MatchKeyword(Keyword_module, false)) {
                stmt = ParseModuleDeclaration();
            } else {
                stmt = ParseStatement();
            }
            
            if (stmt) {
                m_ast_iterator->Push(stmt);
            } else {
                // skip ahead to avoid endlessly looping
                if (m_token_stream->HasNext()) {
                    m_token_stream->Next();
                }
            }
        }
    }
}

int Parser::OperatorPrecedence(const Operator *&out)
{
    out = nullptr;
    
    const Token token = m_token_stream->Peek();

    if (token && token.GetType() == Token::TokenType::Token_operator) {
        if (!Operator::IsBinaryOperator(token.GetValue(), out)) {
            // internal error: operator not defined
            CompilerError error(Level_fatal,
                Msg_internal_error, token.GetLocation());

            m_compilation_unit->GetErrorList().AddError(error);
        }
    }

    if (out) {
        return out->GetPrecedence();
    } else {
        return -1;
    }
}

std::shared_ptr<AstStatement> Parser::ParseStatement(bool top_level)
{
    if (Match(Token::TokenType::Token_keyword, false)) {
        if (MatchKeyword(Keyword_module, false)) {
            auto module_decl = ParseModuleDeclaration();
            
            if (top_level) {
                return module_decl;
            }

            // module may not be declared in a block
            CompilerError error(Level_fatal, 
                Msg_module_declared_in_block, m_token_stream->Next().GetLocation());
            m_compilation_unit->GetErrorList().AddError(error);
            
            return nullptr;
        } else if (MatchKeyword(Keyword_import, false)) {
            return ParseImport();
        } else if (MatchKeyword(Keyword_let, false)) {
            return ParseVariableDeclaration(true);
        } else if (MatchKeyword(Keyword_func, false)) {
            if (MatchAhead(Token::TokenType::Token_identifier, 1)) {
                return ParseFunctionDefinition();
            } else {
                return ParseFunctionExpression();
            }
        } else if (MatchKeyword(Keyword_type, false)) {
            return ParseTypeDefinition();
        } else if (MatchKeyword(Keyword_if, false)) {
            return ParseIfStatement();
        } else if (MatchKeyword(Keyword_while, false)) {
            return ParseWhileLoop();
        } else if (MatchKeyword(Keyword_print, false)) {
            return ParsePrintStatement();
        } else if (MatchKeyword(Keyword_try, false)) {
            return ParseTryCatchStatement();
        } else if (MatchKeyword(Keyword_return, false)) {
            return ParseReturnStatement();
        }
    } else if (Match(Token::TokenType::Token_identifier, false) &&
               MatchAhead(Token::TokenType::Token_colon, 1)) {
        return ParseVariableDeclaration(false);
    } else if (Match(Token::TokenType::Token_open_brace, false)) {
        return ParseBlock();
    }

    return ParseExpression();
}

std::shared_ptr<AstModuleDeclaration> Parser::ParseModuleDeclaration()
{
    if (const Token module_decl = ExpectKeyword(Keyword_module, true)) {
        if (const Token module_name = Expect(Token::TokenType::Token_identifier, true)) {
            // expect open brace
            if (Expect(Token::TokenType::Token_open_brace, true)) {
                std::shared_ptr<AstModuleDeclaration> module_ast(
                    new AstModuleDeclaration(module_name.GetValue(), module_decl.GetLocation()));

                // build up the module declaration with statements
                while (m_token_stream->HasNext() && !Match(Token::TokenType::Token_close_brace, false)) {
                    // skip statement terminator tokens
                    if (!Match(Token::TokenType::Token_semicolon, true) &&
                        !Match(Token::TokenType::Token_newline, true)) {

                        // parse at top level, to allow for nested modules
                        module_ast->AddChild(ParseStatement(true));
                    }
                }

                // expect close brace
                if (Expect(Token::TokenType::Token_close_brace, true)) {
                    return module_ast;
                }
            }
        }
    }

    return nullptr;
}

std::shared_ptr<AstExpression> Parser::ParseTerm()
{
    const Token token = m_token_stream->Peek();
    
    if (!token) {
        CompilerError error(Level_fatal, Msg_unexpected_eof, CurrentLocation());
        m_compilation_unit->GetErrorList().AddError(error);
        if (m_token_stream->HasNext()) {
            m_token_stream->Next();
        }
        return nullptr;
    }

    std::shared_ptr<AstExpression> expr;

    if (Match(Token::TokenType::Token_open_parenthesis)) {
        expr = ParseParentheses();
    } else if (Match(Token::TokenType::Token_open_bracket)) {
        expr = ParseArrayExpression();
    } else if (Match(Token::TokenType::Token_integer_literal)) {
        expr = ParseIntegerLiteral();
    } else if (Match(Token::TokenType::Token_float_literal)) {
        expr = ParseFloatLiteral();
    } else if (Match(Token::TokenType::Token_string_literal)) {
        expr = ParseStringLiteral();
    } else if (Match(Token::TokenType::Token_identifier)) {
        if (MatchAhead(Token::TokenType::Token_double_colon, 1)) {
            expr = ParseModuleAccess();
        } else {
            expr = ParseIdentifier();
        }
    } else if (MatchKeyword(Keyword_true)) {
        expr = ParseTrue();
    } else if (MatchKeyword(Keyword_false)) {
        expr = ParseFalse();
    } else if (MatchKeyword(Keyword_null)) {
        expr = ParseNull();
    } else if (MatchKeyword(Keyword_func)) {
        expr = ParseFunctionExpression();
    } else if (Match(Token::TokenType::Token_operator)) {
        expr = ParseUnaryExpression();
    } else {
        CompilerError error(Level_fatal, Msg_unexpected_token,
            token.GetLocation(), token.GetValue());
        m_compilation_unit->GetErrorList().AddError(error);

        if (m_token_stream->HasNext()) {
            m_token_stream->Next();
        }

        return nullptr;
    }

    if (expr) {
        if (Match(Token::TokenType::Token_dot, false)) {
            expr = ParseMemberAccess(expr);
        }
        if (Match(Token::TokenType::Token_open_bracket, false)) {
            expr = ParseArrayAccess(expr);
        }
    }

    return expr;
}

std::shared_ptr<AstExpression> Parser::ParseParentheses()
{
    SourceLocation location = CurrentLocation();

    Expect(Token::TokenType::Token_open_parenthesis, true);
    std::shared_ptr<AstExpression> expr = ParseExpression();
/*
    // if it's a function expression
    bool is_function_expr = false;

    std::vector<std::shared_ptr<AstParameter>> parameters;
    bool found_variadic = false;

    while (Match(Token::TokenType::Token_comma, true)) {
        is_function_expr = true;

        const Token param_token = Match(Token::TokenType::Token_identifier, true);
        if (!param_token) {
            break;
        }

        // TODO make use of the type specification
        std::shared_ptr<AstTypeSpecification> type_spec;
        std::shared_ptr<AstTypeContractExpression> type_contract;

        // check if parameter type has been declared
        if (Match(Token::TokenType::Token_colon, true)) {
            // type contracts are denoted by <angle brackets>
            if (MatchOperator(&Operator::operator_less, false)) {
                // parse type contracts
                type_contract = ParseTypeContract();
            } else {
                type_spec = ParseTypeSpecification();
            }
        }


        if (found_variadic) {
            // found another parameter after variadic
            m_compilation_unit->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_argument_after_varargs, param_token.GetLocation()));
        }

        bool is_variadic = false;
        if (Match(Token::TokenType::Token_ellipsis, true)) {
            is_variadic = true;
            found_variadic = true;
        }

        auto param = std::shared_ptr<AstParameter>(
            new AstParameter(param_token.GetValue(), is_variadic, param_token.GetLocation()));

        if (type_contract) {
            param->SetTypeContract(type_contract);
        }

        parameters.push_back(param);
    }*/

    Expect(Token::TokenType::Token_close_parenthesis, true);

    /*if (is_function_expr) {
        std::shared_ptr<AstTypeSpecification> return_type_spec;

        // return type specification
        if (Match(Token::TokenType::Token_colon, true)) {
            // read return type for functions
            return_type_spec = ParseTypeSpecification();
        }

        if (auto block = ParseBlock()) {
            return std::shared_ptr<AstFunctionExpression>(
                new AstFunctionExpression(parameters, return_type_spec, block, location));
        }
    }*/

    return expr;
}

std::shared_ptr<AstInteger> Parser::ParseIntegerLiteral()
{
    const Token token = Expect(Token::TokenType::Token_integer_literal, true);
    if (token) {
        a_int value = (a_int)atoll(token.GetValue().c_str());
        return std::shared_ptr<AstInteger>(
            new AstInteger(value, token.GetLocation()));
    }
    return nullptr;
}

std::shared_ptr<AstFloat> Parser::ParseFloatLiteral()
{
    const Token token = Expect(Token::TokenType::Token_float_literal, true);
    if (token) {
        a_float value = (a_float)atof(token.GetValue().c_str());
        return std::shared_ptr<AstFloat>(
            new AstFloat(value, token.GetLocation()));
    }
    return nullptr;
}

std::shared_ptr<AstString> Parser::ParseStringLiteral()
{
    const Token token = Expect(Token::TokenType::Token_string_literal, true);
    if (token) {
        return std::shared_ptr<AstString>(
            new AstString(token.GetValue(), token.GetLocation()));
    }
    return nullptr;
}

std::shared_ptr<AstIdentifier> Parser::ParseIdentifier()
{
    const Token token = Expect(Token::TokenType::Token_identifier, false);
    if (token) {
        if (MatchAhead(Token::TokenType::Token_open_parenthesis, 1)) {
            // function call
            return ParseFunctionCall();
        } else {
            // read identifier token
            if (m_token_stream->HasNext()) {
                m_token_stream->Next();
            }

            // return variable
            return std::shared_ptr<AstVariable>(
                new AstVariable(token.GetValue(), token.GetLocation()));
        }
    }

    return nullptr;
}

std::shared_ptr<AstFunctionCall> Parser::ParseFunctionCall()
{
    const Token token = Expect(Token::TokenType::Token_identifier, true);
    Expect(Token::TokenType::Token_open_parenthesis, true);

    std::vector<std::shared_ptr<AstExpression>> args;

    while (!Match(Token::TokenType::Token_close_parenthesis, false)) {
        SourceLocation expr_location = CurrentLocation();
        std::shared_ptr<AstExpression> expr = ParseExpression();
        if (!expr) {
            CompilerError error(Level_fatal, Msg_illegal_expression, expr_location);
            m_compilation_unit->GetErrorList().AddError(error);
            return nullptr;
        }

        args.push_back(expr);

        if (!Match(Token::TokenType::Token_comma, true)) {
            // unexpected token
            break;
        }
    }

    Expect(Token::TokenType::Token_close_parenthesis, true);

    return std::shared_ptr<AstFunctionCall>(
            new AstFunctionCall(token.GetValue(), args, token.GetLocation()));
}

std::shared_ptr<AstModuleAccess> Parser::ParseModuleAccess()
{
    const Token token = Expect(Token::TokenType::Token_identifier, true);
    Expect(Token::TokenType::Token_double_colon, true);

    std::shared_ptr<AstExpression> expr;

    if (MatchAhead(Token::TokenType::Token_double_colon, 1)) {
        expr = ParseModuleAccess();
    } else {
        expr = ParseIdentifier();
    }

    if (!expr) {
        return nullptr;
    }

    return std::shared_ptr<AstModuleAccess>(
        new AstModuleAccess(token.GetValue(), expr, token.GetLocation()));
}

std::shared_ptr<AstMemberAccess> Parser::ParseMemberAccess(std::shared_ptr<AstExpression> target)
{
    std::vector<std::shared_ptr<AstIdentifier>> parts;

    Expect(Token::TokenType::Token_dot, true);
    do {
        auto ident = ParseIdentifier();
        if (!ident) {
            return nullptr;
        }

        parts.push_back(ident);
    } while (Match(Token::TokenType::Token_dot, true));

    return std::shared_ptr<AstMemberAccess>(
        new AstMemberAccess(target, parts, target->GetLocation()));
}

std::shared_ptr<AstArrayAccess> Parser::ParseArrayAccess(std::shared_ptr<AstExpression> target)
{
    const Token token = Expect(Token::TokenType::Token_open_bracket, true);
    
    if (token) {
        auto expr = ParseExpression();
        Expect(Token::TokenType::Token_close_bracket, true);
        if (expr) {
            return std::shared_ptr<AstArrayAccess>(
                new AstArrayAccess(target, expr, token.GetLocation()));
        }
    }

    return nullptr;
}

std::shared_ptr<AstTrue> Parser::ParseTrue()
{
    const Token token = ExpectKeyword(Keyword_true, true);
    return std::shared_ptr<AstTrue>(
        new AstTrue(token.GetLocation()));
}

std::shared_ptr<AstFalse> Parser::ParseFalse()
{
    const Token token = ExpectKeyword(Keyword_false, true);
    return std::shared_ptr<AstFalse>(
        new AstFalse(token.GetLocation()));
}

std::shared_ptr<AstNull> Parser::ParseNull()
{
    const Token token = ExpectKeyword(Keyword_null, true);
    return std::shared_ptr<AstNull>(
        new AstNull(token.GetLocation()));
}

std::shared_ptr<AstBlock> Parser::ParseBlock()
{
    const Token token = Expect(Token::TokenType::Token_open_brace, true);

    if (token) {
        std::shared_ptr<AstBlock> block(new AstBlock(token.GetLocation()));

        while (!Match(Token::TokenType::Token_close_brace, true)) {
            // skip statement terminator tokens
            if (!Match(Token::TokenType::Token_semicolon, true) &&
                !Match(Token::TokenType::Token_newline, true)) {
                block->AddChild(ParseStatement());
            }
        }

        return block;
    }

    return nullptr;
}

std::shared_ptr<AstIfStatement> Parser::ParseIfStatement()
{
    const Token token = ExpectKeyword(Keyword_if, true);
    
    if (token) {
        SourceLocation cond_location = CurrentLocation();

        std::shared_ptr<AstExpression> conditional = ParseExpression();
        if (conditional == nullptr) {
            return nullptr;
        }

        std::shared_ptr<AstBlock> block = ParseBlock();
        if (block == nullptr) {
            return nullptr;
        }

        std::shared_ptr<AstBlock> else_block = nullptr;
        // parse else statement if the "else" keyword is found
        const Token else_token = MatchKeyword(Keyword_else, true);
        if (else_token) {
            // check for "if" keyword for else-if
            if (MatchKeyword(Keyword_if, false)) {
                else_block = std::shared_ptr<AstBlock>(new AstBlock(else_token.GetLocation()));
                else_block->AddChild(ParseIfStatement());
            } else {
                // parse block after "else keyword
                else_block = ParseBlock();
            }
        }

        return std::shared_ptr<AstIfStatement>(
            new AstIfStatement(conditional, block, else_block,
                token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstWhileLoop> Parser::ParseWhileLoop()
{
    const Token token = ExpectKeyword(Keyword_while, true);
    if (token) {
        SourceLocation cond_location = CurrentLocation();
        std::shared_ptr<AstExpression> conditional = ParseExpression();
        std::shared_ptr<AstBlock> block = ParseBlock();

        if (conditional == nullptr) {
            CompilerError error(Level_fatal, Msg_illegal_expression, cond_location);
            m_compilation_unit->GetErrorList().AddError(error);
            return nullptr;
        }
        if (block == nullptr) {
            return nullptr;
        }

        return std::shared_ptr<AstWhileLoop>(
            new AstWhileLoop(conditional, block, token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstPrintStatement> Parser::ParsePrintStatement()
{
    const Token token = ExpectKeyword(Keyword_print, true);
    if (token) {
        std::vector<std::shared_ptr<AstExpression>> arguments;

        while (true) {
            SourceLocation loc = CurrentLocation();
            auto expr = ParseExpression();
            if (expr == nullptr) {
                // expression or statement could not be evaluated
                CompilerError error(Level_fatal, Msg_illegal_expression, loc);
                m_compilation_unit->GetErrorList().AddError(error);
            } else {
                arguments.push_back(expr);
            }

            if (!Match(Token::TokenType::Token_comma, true)) {
                break;
            }
        }

        return std::shared_ptr<AstPrintStatement>(
            new AstPrintStatement(arguments, token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstTryCatch> Parser::ParseTryCatchStatement()
{
    const Token token = ExpectKeyword(Keyword_try, true);
    if (token) {
        std::shared_ptr<AstBlock> try_block(ParseBlock());
        std::shared_ptr<AstBlock> catch_block(nullptr);

        if (ExpectKeyword(Keyword_catch, true)) {
            // TODO: Add exception argument
            catch_block = ParseBlock();
        }

        if (try_block == nullptr || catch_block == nullptr) {
            return nullptr;
        }

        return std::shared_ptr<AstTryCatch>(
            new AstTryCatch(try_block, catch_block, token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstExpression> Parser::ParseBinaryExpression(int expr_prec,
    std::shared_ptr<AstExpression> left)
{
    while (true) {
        // get precedence
        const Operator *op = nullptr;
        int precedence = OperatorPrecedence(op);
        if (precedence < expr_prec) {
            return left;
        }

        // read the operator token
        const Token token = Expect(Token::TokenType::Token_operator, true);

        std::shared_ptr<AstExpression> right = ParseTerm();
        if (right == nullptr) {
            return nullptr;
        }

        // next part of expression's precedence
        const Operator *next_op = nullptr;
        int next_prec = OperatorPrecedence(next_op);
        if (precedence < next_prec) {
            right = ParseBinaryExpression(precedence + 1, right);
            if (right == nullptr) {
                return nullptr;
            }
        }

        left = std::shared_ptr<AstBinaryExpression>(
            new AstBinaryExpression(left, right, op,
                token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstExpression> Parser::ParseUnaryExpression()
{
    // read the operator token
    const Token token = Expect(Token::TokenType::Token_operator, true);

    if (token) {
        const Operator *op = nullptr;
        if (Operator::IsUnaryOperator(token.GetValue(), op)) {
            auto term = ParseTerm();
            if (!term) {
                return nullptr;
            }

            return std::shared_ptr<AstUnaryExpression>(
                new AstUnaryExpression(term, op, token.GetLocation()));
        } else {
            // internal error: operator not defined
            CompilerError error(Level_fatal,
                Msg_illegal_operator, token.GetLocation(), token.GetValue());
            m_compilation_unit->GetErrorList().AddError(error);
        }
    }

    return nullptr;
}

std::shared_ptr<AstExpression> Parser::ParseExpression()
{
    std::shared_ptr<AstExpression> term = ParseTerm();
    if (term == nullptr) {
        return nullptr;
    }

    if (Match(Token::TokenType::Token_operator, false)) {
        std::shared_ptr<AstExpression> bin_expr = ParseBinaryExpression(0, term);
        if (bin_expr == nullptr) {
            return nullptr;
        }
        term = bin_expr;
    }

    return term;
}

std::shared_ptr<AstTypeSpecification> Parser::ParseTypeSpecification()
{
    if (const Token left = Expect(Token::TokenType::Token_identifier, true)) {
        std::shared_ptr<AstTypeSpecification> right(nullptr);

        // module access of type
        if (Match(Token::TokenType::Token_double_colon, true)) {
            // read next part
            right = ParseTypeSpecification();
        }

        return std::shared_ptr<AstTypeSpecification>(
            new AstTypeSpecification(left.GetValue(), right, left.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstTypeContractExpression> Parser::ParseTypeContract()
{
    std::shared_ptr<AstTypeContractExpression> expr;

    // we have to use ExpectOperator for angle brackets
    const Token token = ExpectOperator(&Operator::operator_less, true);
    if (token) {
        expr = ParseTypeContractExpression();
    }

    ExpectOperator(&Operator::operator_greater, true);

    return expr;
}

std::shared_ptr<AstTypeContractExpression> Parser::ParseTypeContractExpression()
{
    auto term = ParseTypeContractTerm();
    if (!term) {
        return nullptr;
    }

    if (Match(Token::TokenType::Token_operator, false)) {
        auto expr = ParseTypeContractBinaryExpression(0, term);
        if (!expr) {
            return nullptr;
        }
        term = expr;
    }

    return term;
}


std::shared_ptr<AstTypeContractExpression> Parser::ParseTypeContractTerm()
{
    const Token contract_prop = Expect(Token::TokenType::Token_identifier, true);
    if (contract_prop) {
        // read the contract parameter here
        return std::shared_ptr<AstTypeContractTerm>(new AstTypeContractTerm(
            contract_prop.GetValue(), ParseTypeSpecification(), contract_prop.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstTypeContractExpression> Parser::ParseTypeContractBinaryExpression(int expr_prec,
    std::shared_ptr<AstTypeContractExpression> left)
{
    while (true) {
        // check for end of contract
        if (MatchOperator(&Operator::operator_greater, false)) {
            return left;
        }

        // get precedence
        const Operator *op = nullptr;
        int precedence = OperatorPrecedence(op);
        if (precedence < expr_prec) {
            return left;
        }

        // read the operator token
        const Token token = Expect(Token::TokenType::Token_operator, false);
        Token token_op = MatchOperator(&Operator::operator_bitwise_or, true);
        if (!token) {
            return nullptr;
        }
        if (!token_op) {
            // try and operator
            token_op = MatchOperator(&Operator::operator_bitwise_and, true);
        }
        if (!token_op) {
            CompilerError error(Level_fatal, Msg_invalid_type_contract_operator, token.GetLocation(), token.GetValue());
            m_compilation_unit->GetErrorList().AddError(error);
            return nullptr;
        }

        auto right = ParseTypeContractTerm();
        if (!right) {
            return nullptr;
        }

        // next part of expression's precedence
        const Operator *next_op = nullptr;
        int next_prec = OperatorPrecedence(next_op);
        if (precedence < next_prec) {
            right = ParseTypeContractBinaryExpression(precedence + 1, right);
            if (!right) {
                return nullptr;
            }
        }

        left = std::shared_ptr<AstTypeContractBinaryExpression>(
            new AstTypeContractBinaryExpression(left, right, op, token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstVariableDeclaration> Parser::ParseVariableDeclaration(bool require_keyword)
{
    SourceLocation location = CurrentLocation();

    if (require_keyword) {
        if (!ExpectKeyword(Keyword_let, true)) {
            return nullptr;
        }
    } else {
        // match and read in the case that it is found
        MatchKeyword(Keyword_let, true);
    }

    const Token identifier = Expect(Token::TokenType::Token_identifier, true);
    if (identifier) {
        std::shared_ptr<AstTypeSpecification> type_spec;

        if (Match(Token::TokenType::Token_colon, true)) {
            // read object type
            type_spec = ParseTypeSpecification();
        }

        std::shared_ptr<AstExpression> assignment;

        const Token op = Match(Token::TokenType::Token_operator, true);
        if (op) {
            if (op.GetValue() == Operator::operator_assign.ToString()) {
                // read assignment expression
                SourceLocation expr_location = CurrentLocation();
                assignment = ParseExpression();
                if (!assignment) {
                    CompilerError error(Level_fatal, Msg_illegal_expression, expr_location);
                    m_compilation_unit->GetErrorList().AddError(error);
                }
            } else {
                // unexpected operator
                CompilerError error(Level_fatal,
                    Msg_illegal_operator, op.GetLocation());

                m_compilation_unit->GetErrorList().AddError(error);
            }
        }

        return std::shared_ptr<AstVariableDeclaration>(
            new AstVariableDeclaration(identifier.GetValue(),
                type_spec, assignment, location));
    }

    return nullptr;
}

std::shared_ptr<AstFunctionDefinition> Parser::ParseFunctionDefinition(bool require_keyword)
{
    SourceLocation location = CurrentLocation();

    if (require_keyword) {
        if (!ExpectKeyword(Keyword_func, true)) {
            return nullptr;
        }
    } else {
        // match and read in the case that it is found
        MatchKeyword(Keyword_func, true);
    }

    const Token identifier = Expect(Token::TokenType::Token_identifier, true);

    if (identifier) {
        auto expr = ParseFunctionExpression(false, ParseFunctionParameters());
        if (expr) {
            return std::shared_ptr<AstFunctionDefinition>(
                new AstFunctionDefinition(identifier.GetValue(), expr, location));
        }
    }

    return nullptr;
}

std::shared_ptr<AstFunctionExpression> Parser::ParseFunctionExpression(bool require_keyword,
    std::vector<std::shared_ptr<AstParameter>> params)
{
    const Token token = require_keyword ? ExpectKeyword(Keyword_func, true) : Token::EMPTY;
    SourceLocation location = token ? token.GetLocation() : CurrentLocation();

    if (require_keyword || !token) {
        if (require_keyword) {
            // read params
            params = ParseFunctionParameters();
        }

        std::shared_ptr<AstTypeSpecification> type_spec;

        if (Match(Token::TokenType::Token_colon, true)) {
            // read return type for functions
            type_spec = ParseTypeSpecification();
        }

        // parse function block
        auto block = ParseBlock();
        if (block) {
            return std::shared_ptr<AstFunctionExpression>(new AstFunctionExpression(
                params, type_spec, block, location));
        }
    }

    return nullptr;
}

std::shared_ptr<AstArrayExpression> Parser::ParseArrayExpression()
{
    const Token token = Expect(Token::TokenType::Token_open_bracket, true);

    if (token) {
        std::vector<std::shared_ptr<AstExpression>> members;

        while (true) {
            if (Match(Token::TokenType::Token_close_bracket, false)) {
                break;
            }

            auto expr = ParseExpression();
            if (expr) {
                members.push_back(expr);
            }

            if (!Match(Token::TokenType::Token_comma, true)) {
                break;
            }
        }

        Expect(Token::TokenType::Token_close_bracket, true);

        return std::shared_ptr<AstArrayExpression>(
            new AstArrayExpression(members, token.GetLocation()));
    }

    return nullptr;
}

std::vector<std::shared_ptr<AstParameter>> Parser::ParseFunctionParameters()
{
    std::vector<std::shared_ptr<AstParameter>> parameters;

    if (Match(Token::TokenType::Token_open_parenthesis, true)) {
        bool found_variadic = false;

        while (true) {
            const Token tok = Match(Token::TokenType::Token_identifier, true);
            if (tok) {
                // TODO make use of the type specification
                std::shared_ptr<AstTypeSpecification> type_spec;
                std::shared_ptr<AstTypeContractExpression> type_contract;
                // check if parameter type has been declared
                if (Match(Token::TokenType::Token_colon, true)) {
                    //if (Match(Token::TokenType::Token_open_angle_bracket, false)) {
                        // parse type contracts
                        type_contract = ParseTypeContract();
                    //} else {
                    //    type_spec = ParseTypeSpecification();
                    //}
                }


                if (found_variadic) {
                    // found another parameter after variadic
                    CompilerError error(Level_fatal,
                        Msg_argument_after_varargs, tok.GetLocation());

                    m_compilation_unit->GetErrorList().AddError(error);
                }

                bool is_variadic = false;
                if (Match(Token::TokenType::Token_ellipsis, true)) {
                    is_variadic = true;
                    found_variadic = true;
                }

                auto param = std::shared_ptr<AstParameter>(
                    new AstParameter(tok.GetValue(), is_variadic, tok.GetLocation()));

                if (type_contract != nullptr) {
                    param->SetTypeContract(type_contract);
                }

                parameters.push_back(param);

                if (!Match(Token::TokenType::Token_comma, true)) {
                    break;
                }
            } else {
                break;
            }
        }

        Expect(Token::TokenType::Token_close_parenthesis, true);
    }

    return parameters;
}

std::shared_ptr<AstTypeDefinition> Parser::ParseTypeDefinition()
{
    const Token token = ExpectKeyword(Keyword_type, true);
    const Token identifier = Expect(Token::TokenType::Token_identifier, true);

    if (token && identifier) {
        std::vector<std::shared_ptr<AstVariableDeclaration>> members;

        if (Expect(Token::TokenType::Token_open_brace, true)) {
            while (!Match(Token::TokenType::Token_close_brace, true)) {
                SkipStatementTerminators();

                if (MatchKeyword(Keyword_let, false) || Match(Token::TokenType::Token_identifier, false)) {
                    // do not require keyword for data members
                    members.push_back(ParseVariableDeclaration(false));
                } else {
                    // error; unexpected token
                    CompilerError error(Level_fatal, Msg_unexpected_token,
                        m_token_stream->Peek().GetLocation(),
                        m_token_stream->Peek().GetValue());
                    m_compilation_unit->GetErrorList().AddError(error);

                    if (m_token_stream->HasNext()) {
                        m_token_stream->Next();
                    }
                }

                SkipStatementTerminators();
            }
        }

        return std::shared_ptr<AstTypeDefinition>(
            new AstTypeDefinition(identifier.GetValue(), members, token.GetLocation()));
    }

    return nullptr;
}

std::shared_ptr<AstImport> Parser::ParseImport()
{
    if (ExpectKeyword(Keyword_import, true)) {
        if (Match(Token::TokenType::Token_string_literal, false)) {
            return ParseLocalImport();
        } else {
            // TODO: handle other types of imports here
        }
    }

    return nullptr;
}

std::shared_ptr<AstLocalImport> Parser::ParseLocalImport()
{
    SourceLocation location = CurrentLocation();

    const Token file = Expect(Token::TokenType::Token_string_literal, true);
    
    if (file) {
        std::shared_ptr<AstLocalImport> result(
            new AstLocalImport(file.GetValue(), location));

        return result;
    }

    return nullptr;
}

std::shared_ptr<AstReturnStatement> Parser::ParseReturnStatement()
{
    SourceLocation location = CurrentLocation();

    const Token token = ExpectKeyword(Keyword_return, true);
    
    if (token) {
        SourceLocation expr_location = CurrentLocation();
        auto expr = ParseExpression();
        if (!expr) {
            CompilerError error(Level_fatal, Msg_illegal_expression, expr_location);
            m_compilation_unit->GetErrorList().AddError(error);
        }
        return std::shared_ptr<AstReturnStatement>(new AstReturnStatement(expr, location));
    }

    return nullptr;
}
