#include <ace-c/compiler_error.hpp>

const std::map<ErrorMessage, std::string> CompilerError::error_message_strings {
    /* Fatal errors */
    { Msg_internal_error, "internal error" },
    { Msg_illegal_syntax, "illegal syntax" },
    { Msg_illegal_expression, "illegal expression" },
    { Msg_illegal_operator, "illegal usage of operator '%'" },
    { Msg_invalid_operator_for_type, "operator '%' is not valid for type '%'" },
    { Msg_const_modified, "'%' is immutable and cannot be modified" },
    { Msg_cannot_modify_rvalue, "The left hand side is not suitable for assignment" },
    { Msg_prohibited_action_attribute, "attribute '%' prohibits this action" },
    { Msg_unbalanced_expression, "unbalanced expression" },
    { Msg_unexpected_character, "unexpected character '%'" },
    { Msg_unexpected_identifier, "unexpected identifier '%'" },
    { Msg_unexpected_token, "unexpected token '%'" },
    { Msg_unexpected_eof, "unexpected end of file" },
    { Msg_unrecognized_escape_sequence, "unrecognized escape sequence '%'" },
    { Msg_unterminated_string_literal, "unterminated string literal" },
    { Msg_argument_after_varargs, "argument not allowed after '...'" },
    { Msg_too_many_args, "too many arguments used for function '%'" },
    { Msg_too_few_args, "too few arguments used for function '%'" },
    { Msg_redeclared_identifier, "identifier '%' has already been declared in this scope" },
    { Msg_redeclared_identifier_module, "'%' is the name of a module and cannot be used as a symbol name" },
    { Msg_undeclared_identifier, "'%' has not been declared in this scope" },
    { Msg_expected_identifier, "expected an identifier" },
    { Msg_ambiguous_identifier, "identifier '%' is ambiguous" },
    { Msg_invalid_constructor, "invalid constructor" },
    { Msg_expected_type_got_identifier, "'%' is an identifier, expected a type" },
    { Msg_missing_type_and_assignment, "no type or assignment has been provided for '%'" },
    { Msg_multiple_return_types, "function has more than one possible return type" },
    { Msg_mismatched_return_type, "function is marked to return '%', cannot return '%'" },
    { Msg_must_be_explicitly_marked_any, "function must be explicitly marked to return 'Any'" },
    { Msg_return_outside_function, "'return' not allowed outside of function body" },
    { Msg_not_a_function, "'%' is not a function" },
    { Msg_undefined_type, "'%' is not a built-in or user-defined type" },
    { Msg_redefined_type, "type '%' has already been defined in this module" },
    { Msg_redefined_builtin_type, "cannot create type '%', it is a built-in type" },
    { Msg_mismatched_types, "mismatched types '%' and '%'" },
    { Msg_not_a_data_member, "'%' is not a member of type '%'" },
    { Msg_bitwise_operands_must_be_int, "bitwise operands must both be 'Int', got '%' and '%'" },
    { Msg_bitwise_operand_must_be_int, "bitwise operand must be 'Int', got '%'" },
    { Msg_expected_token, "expected '%'" },
    { Msg_unknown_module, "'%' is not an imported module" },
    { Msg_expected_module, "expected 'module' declaration, cannot continue" },
    { Msg_empty_module, "the module is empty" },
    { Msg_module_already_defined, "module '%' was already defined or imported" },
    { Msg_module_not_imported, "module '%' was not imported" },
    { Msg_identifier_is_module, "'%' is the name of a module, expected an identifier" },
    { Msg_invalid_module_access, "'%' is a module, expected an identifier or function call" },
    { Msg_could_not_open_file, "could not open file '%'" },
    { Msg_import_outside_global, "import not allowed outside of global scope" },
    { Msg_import_current_file, "attempt to import current file" },
    { Msg_self_outside_class, "'self' not allowed outside of a class" },
    { Msg_else_outside_if, "'else' not connected to an if statement" },
    { Msg_alias_missing_assignment, "alias '%' must have an assignment" },
    { Msg_alias_must_be_identifier, "alias '%' must reference an identifier" },
    { Msg_unrecognized_alias_type, "unrecognized alias type for '%'" },
    { Msg_unsupported_feature, "unsupported feature" },

    /* Warnings */
    { Msg_unreachable_code, "unreachable code detected" },
    { Msg_expected_semicolon, "missing semicolon" },

    /* Info */
    { Msg_unused_identifier, "'%' is not used" },
    { Msg_empty_function_body, "the function body of '%' is empty" },
    { Msg_empty_statement_body, "loop or statement body is empty" },
    { Msg_module_name_begins_lowercase, "module name '%' should begin with an uppercase character" },
};

bool CompilerError::operator<(const CompilerError &other) const
{
    if (m_level == other.m_level) {
        return m_location < other.m_location;
    } else {
        return m_level < other.m_level;
    }
}
