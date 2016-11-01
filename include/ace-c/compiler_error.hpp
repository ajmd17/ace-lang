#ifndef COMPILER_ERROR_HPP
#define COMPILER_ERROR_HPP

#include <ace-c/source_location.hpp>

#include <string>
#include <sstream>
#include <map>

enum ErrorLevel {
    Level_info,
    Level_warning,
    Level_fatal
};

enum ErrorMessage {
    /* Fatal errors */
    Msg_internal_error,
    Msg_illegal_syntax,
    Msg_illegal_expression,
    Msg_illegal_operator,
    Msg_const_modified,
    Msg_cannot_modify_rvalue,
    Msg_prohibited_action_attribute,
    Msg_unbalanced_expression,
    Msg_unexpected_character,
    Msg_unexpected_identifier,
    Msg_unexpected_token,
    Msg_unexpected_eof,
    Msg_unrecognized_escape_sequence,
    Msg_unterminated_string_literal,
    Msg_argument_after_varargs,
    Msg_too_many_args,
    Msg_too_few_args,
    Msg_redeclared_identifier,
    Msg_undeclared_identifier,
    Msg_expected_identifier,
    Msg_ambiguous_identifier,
    Msg_invalid_constructor,
    Msg_expected_type_got_identifier,
    Msg_missing_type_and_assignment,
    Msg_multiple_return_types,
    Msg_mismatched_return_type,
    Msg_must_be_explicitly_marked_any,
    Msg_return_outside_function,
    Msg_not_a_function,
    Msg_undefined_type,
    Msg_redefined_type,
    Msg_mismatched_types,
    Msg_not_a_data_member,
    Msg_bitwise_operands_must_be_int,
    Msg_bitwise_operand_must_be_int,
    Msg_expected_token,
    Msg_unknown_module,
    Msg_expected_module,
    Msg_empty_module,
    Msg_module_already_defined,
    Msg_module_not_imported,
    Msg_invalid_module_access,
    Msg_could_not_open_file,
    Msg_identifier_is_module,
    Msg_import_outside_global,
    Msg_import_current_file,
    Msg_self_outside_class,
    Msg_else_outside_if,
    Msg_alias_missing_assignment,
    Msg_alias_must_be_identifier,
    Msg_unrecognized_alias_type,
    Msg_unsupported_feature,

    /* Warnings */
    Msg_unreachable_code,
    Msg_expected_semicolon,

    /* Info */
    Msg_unused_identifier,
    Msg_empty_function_body,
    Msg_empty_statement_body,
    Msg_module_name_begins_lowercase,
};

class CompilerError {
    static const std::map<ErrorMessage, std::string> error_message_strings;

public:
    template <typename ... Args>
    CompilerError(ErrorLevel level, ErrorMessage msg,
        const SourceLocation &location, const Args &...args)
        : m_level(level),
          m_msg(msg),
          m_location(location)
    {
        switch (m_level) {
        case Level_info:
            m_text = "INFO: ";
            break;
        case Level_warning:
            m_text = "WARNING: ";
            break;
        case Level_fatal:
            m_text = "FATAL: ";
            break;
        }

        MakeMessage(error_message_strings.at(m_msg).c_str(), args...);
    }

    ~CompilerError() {}

    inline ErrorLevel GetLevel() const { return m_level; }
    inline ErrorMessage GetMessage() const { return m_msg; }
    inline const SourceLocation &GetLocation() const { return m_location; }
    inline const std::string &GetText() const { return m_text; }

    bool operator<(const CompilerError &other) const;

private:
    void MakeMessage(const char *format)
    {
        m_text += format;
    }

    template <typename T, typename ... Args>
    void MakeMessage(const char *format, T value, Args && ... args)
    {
        for (; *format != '\0'; format++) {
            if (*format == '%') {
                std::stringstream sstream;
                sstream << value;
                m_text += sstream.str();
                MakeMessage(format + 1, args...);
                return;
            }
            m_text += *format;
        }
    }

    ErrorLevel m_level;
    ErrorMessage m_msg;
    SourceLocation m_location;
    std::string m_text;
};

#endif
