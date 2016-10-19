#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <map>
#include <string>

enum OperatorType {
    ARITHMETIC,
    BITWISE,
    LOGICAL,
    COMPARISON,
    ASSIGNMENT,
};

class Operator {
public:
    static const Operator
        operator_add,
        operator_subtract,
        operator_multiply,
        operator_divide,
        operator_modulus,

        operator_bitwise_xor,
        operator_bitwise_and,
        operator_bitwise_or,
        operator_bitshift_left,
        operator_bitshift_right,

        operator_logical_and,
        operator_logical_or,

        operator_equals,
        operator_not_eql,
        operator_less,
        operator_greater,
        operator_less_eql,
        operator_greater_eql,

        operator_assign,
        operator_add_assign,
        operator_subtract_assign,
        operator_multiply_assign,
        operator_divide_assign,
        operator_modulus_assign,
        operator_bitwise_xor_assign,
        operator_bitwise_and_assign,
        operator_bitwise_or_assign,

        operator_logical_not,
        operator_negative,
        operator_positive,
        operator_bitwise_complement,
        operator_increment,
        operator_decrement;

    static bool IsOperator(const std::string &str);
    static bool IsOperator(const std::string &str, const Operator *&out);
    static bool IsBinaryOperator(const std::string &str);
    static bool IsBinaryOperator(const std::string &str, const Operator *&out);
    static bool IsUnaryOperator(const std::string &str);
    static bool IsUnaryOperator(const std::string &str, const Operator *&out);

public:
    Operator(const std::string &str,
        int precedence, OperatorType type, bool modifies_value = false);
    Operator(const Operator &other) = delete;

    inline const std::string &ToString() const { return m_str; }
    inline int GetPrecedence() const { return m_precedence; }
    inline bool IsUnary() const { return m_precedence == 0; }
    inline OperatorType GetType() const { return m_type; }
    inline bool ModifiesValue() const { return m_modifies_value; }

private:
    std::string m_str;
    int m_precedence;
    OperatorType m_type;
    bool m_modifies_value;
};

#endif
