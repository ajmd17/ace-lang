#include <athens/parser.h>

#include <memory>
#include <cstdlib>

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

const Token *Parser::Match(TokenType type, bool read)
{
    const Token *peek = m_token_stream->Peek();
    if (peek != nullptr && peek->GetType() == type) {
        if (read) {
            m_token_stream->Next();
        }
        return peek;
    }
    return nullptr;
}

const Token *Parser::MatchKeyword(Keywords keyword, bool read)
{
    const Token *peek = m_token_stream->Peek();
    if (peek != nullptr && peek->GetType() == Token_keyword) {
        std::string str = Keyword::ToString(keyword);
        if (peek->GetValue() == str) {
            if (read) {
                m_token_stream->Next();
            }
            return peek;
        }
    }
    return nullptr;
}

const Token *Parser::Expect(TokenType type, bool read)
{
    const Token *token = Match(type, read);
    if (token == nullptr) {
        SourceLocation location = m_token_stream->Peek()->GetLocation();
        if (read) {
            m_token_stream->Next();
        }

        ErrorMessage error_msg;
        std::string error_str;

        switch (type) {
        case Token_identifier:
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

const Token *Parser::ExpectKeyword(Keywords keyword, bool read)
{
    const Token *token = MatchKeyword(keyword, read);
    if (token == nullptr) {
        SourceLocation location = m_token_stream->Peek()->GetLocation();
        if (read) {
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

const SourceLocation &Parser::CurrentLocation() const
{
    const Token *peek = m_token_stream->Peek();
    return peek != nullptr ? peek->GetLocation() : SourceLocation::eof;
}

void Parser::Parse()
{
    std::shared_ptr<AstModuleDeclaration> module_ast(nullptr);

    // all source code files must start with module declaration
    const Token *module_decl = ExpectKeyword(Keyword_module, true);
    if (module_decl != nullptr) {
        const Token *module_name = Expect(Token_identifier, true);
        if (module_name != nullptr) {
            module_ast.reset(new AstModuleDeclaration(
                module_name->GetValue(), module_decl->GetLocation()));

            m_ast_iterator->Push(module_ast);
        }
    }

    while (m_token_stream->HasNext()) {
        SourceLocation location(CurrentLocation());

        std::shared_ptr<AstStatement> statement(ParseStatement());

        if (statement != nullptr) {
            m_ast_iterator->Push(statement);
        } else {
            // expression or statement could not be evaluated
            CompilerError error(Level_fatal,
            Msg_illegal_expression, location);

            m_compilation_unit->GetErrorList().AddError(error);

            // skip ahead to avoid endlessly looping
            m_token_stream->Next();
        }
    }
}

std::shared_ptr<AstStatement> Parser::ParseStatement()
{
    if (Match(Token_keyword, false)) {
        if (MatchKeyword(Keyword_import, false)) {
            return ParseImport();
        } else {
            return nullptr;
        }
    } else {
        return ParseExpression(true);
    }
}

std::shared_ptr<AstExpression> Parser::ParseTerm()
{
    const Token *token = m_token_stream->Peek();
    if (token == nullptr) {
        m_token_stream->Next();
        return nullptr;
    }

    if (Match(Token_open_parenthesis)) {
        return ParseParentheses();
    } else if (Match(Token_integer_literal)) {
        return ParseIntegerLiteral();
    } else if (Match(Token_float_literal)) {
        return ParseFloatLiteral();
    } else if (Match(Token_string_literal)) {
        return ParseStringLiteral();
    } else if (Match(Token_identifier)) {
        return nullptr;//ParseIdentifier();
    } else if (MatchKeyword(Keyword_true)) {
        return nullptr;//ParseTrue();
    } else if (MatchKeyword(Keyword_false)) {
        return nullptr;//ParseFalse();
    } else if (MatchKeyword(Keyword_null)) {
        return nullptr;//ParseNull();
    } else if (MatchKeyword(Keyword_func)) {
        return nullptr;//ParseFunctionExpression();
    } else if (Match(Token_operator)) {
        return nullptr;//ParseUnaryExpression(); 
    } else {
        CompilerError error(Level_fatal, Msg_unexpected_token, 
            token->GetLocation(), token->GetValue());
        m_compilation_unit->GetErrorList().AddError(error);

        m_token_stream->Next();
        
        return nullptr;
    }
}

std::shared_ptr<AstExpression> Parser::ParseParentheses()
{
    Expect(Token_open_parenthesis, true);
    std::shared_ptr<AstExpression> expr(ParseExpression());
    Expect(Token_close_parenthesis, true);
    return expr;
}

std::shared_ptr<AstInteger> Parser::ParseIntegerLiteral()
{
    const Token *token = Expect(Token_integer_literal, true);
    a_int value = (a_int)atoll(token->GetValue().c_str());
    return std::shared_ptr<AstInteger>(
        new AstInteger(value, token->GetLocation()));
}

std::shared_ptr<AstFloat> Parser::ParseFloatLiteral()
{
    const Token *token = Expect(Token_float_literal, true);
    a_float value = (a_float)atof(token->GetValue().c_str());
    return std::shared_ptr<AstFloat>(
        new AstFloat(value, token->GetLocation()));
}

std::shared_ptr<AstString> Parser::ParseStringLiteral()
{
    const Token *token = Expect(Token_string_literal, true);
    return std::shared_ptr<AstString>(
        new AstString(token->GetValue(), token->GetLocation()));
}

std::shared_ptr<AstExpression> Parser::ParseBinaryExpression(int expr_prec, 
    std::shared_ptr<AstExpression> left)
{
    while (true) {
        // get precedence
        int precedence = -1;

        const Operator *op = nullptr;
        const Token *token_op = m_token_stream->Peek();
        if (token_op != nullptr) {
            if (Operator::IsBinaryOperator(token_op->GetValue(), op)) {
                precedence = op->GetPrecedence();
            } else {
                // illegal operator
                CompilerError error(Level_fatal, Msg_illegal_operator, 
                    token_op->GetLocation(), token_op->GetValue());
                m_compilation_unit->GetErrorList().AddError(error);
            }
        }

        if (precedence < expr_prec) {
            return left;
        } else {
            // read the operator
            m_token_stream->Next();

            std::shared_ptr<AstExpression> right = ParseTerm();
            if (right == nullptr) {
                return nullptr;
            }

            // next part of expression's precedence
            int next_prec = -1;

            const Token *token_next = m_token_stream->Peek();
            if (token_next != nullptr) {
                const Operator *op_next = nullptr;
                if (Operator::IsBinaryOperator(token_next->GetValue(), op_next)) {
                    next_prec = op_next->GetPrecedence();
                } else {
                    // illegal operator
                    CompilerError error(Level_fatal, Msg_illegal_operator, 
                        token_next->GetLocation(), token_next->GetValue());
                    m_compilation_unit->GetErrorList().AddError(error);
                }
            }

            if (precedence < next_prec) {
                right = ParseBinaryExpression(precedence + 1, right);
                if (right == nullptr) {
                    return nullptr;
                }
            }

            left = std::shared_ptr<AstBinaryExpression>(
                new AstBinaryExpression(left, right, op, 
                    token_op->GetLocation()));
        }
    }
    
    return nullptr;
}

std::shared_ptr<AstExpression> Parser::ParseExpression(bool standalone)
{
    std::shared_ptr<AstExpression> term = ParseTerm();
    if (term == nullptr) {
        return nullptr;
    }

    if (Match(Token_operator, false)) {
        std::shared_ptr<AstExpression> bin_expr = ParseBinaryExpression(0, term);
        if (bin_expr == nullptr) {
            return nullptr;
        }
        term = bin_expr;
    }
    
    term->m_is_standalone = standalone;
    return term;
}

std::shared_ptr<AstImport> Parser::ParseImport()
{
    if (ExpectKeyword(Keyword_import, true)) {
        if (Match(Token_string_literal, false)) {
            return ParseLocalImport();
        } else {
            // TODO: handle other types of imports here
        }
    }

    return nullptr;
}

std::shared_ptr<AstLocalImport> Parser::ParseLocalImport()
{
    SourceLocation location(CurrentLocation());

    const Token *file = Expect(Token_string_literal, true);
    if (file != nullptr) {
        std::shared_ptr<AstLocalImport> result(
            new AstLocalImport(file->GetValue(), location));

        return result;
    }

    return nullptr;
}