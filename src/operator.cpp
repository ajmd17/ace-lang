#include <athens/operator.h>

const Operator
    // Arithmetic operators
    Operator::operator_add("+", 11),
    Operator::operator_subtract("-", 11),
    Operator::operator_multiply("*", 12),
    Operator::operator_divide("/", 12),
    Operator::operator_modulus("%", 12),
    
    // Bitwise operators
    Operator::operator_bitwise_xor("^", 6),
    Operator::operator_bitwise_and("&", 7),
    Operator::operator_bitwise_or("|", 5),
    Operator::operator_bitshift_left("<<", 10),
    Operator::operator_bitshift_right(">>", 10),
    
    // Logical operators
    Operator::operator_logical_and("&&", 4),
    Operator::operator_logical_or("||", 3),

    // Comparison operators
    Operator::operator_equals("==", 8),
    Operator::operator_not_eql("!=", 8),
    Operator::operator_less("<", 9),
    Operator::operator_greater(">", 9),
    Operator::operator_less_eql("<=", 9),
    Operator::operator_greater_eql(">=", 9),

    // Assignment operators
    Operator::operator_assign("=", 2),
    Operator::operator_add_assign("+=", 2),
    Operator::operator_subtract_assign("-=", 2),
    Operator::operator_multiply_assign("*=", 2),
    Operator::operator_divide_assign("/=", 2),
    Operator::operator_modulus_assign("%=", 2),
    Operator::operator_bitwise_xor_assign("^=", 2),
    Operator::operator_bitwise_and_assign("&=", 2),
    Operator::operator_bitwise_or_assign("|=", 2),

    // Unary operators
    Operator::operator_logical_not("!", 0),
    Operator::operator_negative("-", 0),
    Operator::operator_positive("+", 0),
    Operator::operator_bitwise_complement("~", 0),
    Operator::operator_increment("++", 0),
    Operator::operator_decrement("--", 0);

Operator::Operator(const std::string &str, int precedence)
    : m_str(str),
      m_precedence(precedence)
{
}
