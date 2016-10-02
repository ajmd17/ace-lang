#include <athens/parser.h>
#include <athens/ast/ast_module_declaration.h>

#include <memory>

Parser::Parser(TokenStream *token_stream, CompilationUnit *compilation_unit, 
        AstIterator *ast_iterator)
    : m_token_stream(token_stream),
      m_compilation_unit(compilation_unit),
      m_ast_iterator(ast_iterator)
{
}

Parser::Parser(const Parser &other)
    : m_token_stream(other.m_token_stream),
      m_compilation_unit(other.m_compilation_unit),
      m_ast_iterator(other.m_ast_iterator)
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
            error_str = "";
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
            error_str = "";
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
}