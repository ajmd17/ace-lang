#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_constant.h>
#include <athens/ast_visitor.h>

static void OptimizeSide(std::shared_ptr<AstExpression> &side)
{
    side->Optimize();

    auto side_as_var = std::dynamic_pointer_cast<AstVariable>(side);
    if (side_as_var != nullptr) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (side_as_var->GetIdentifier() != nullptr) {
            if (side_as_var->GetIdentifier()->GetFlags() & (Flag_const | Flag_literal)) {
                // the variable is a const literal so we can inline it
                // set it to be the current value
                auto sp = side_as_var->GetIdentifier()->GetCurrentValue().lock();
                if (sp != nullptr) {
                    side = sp;
                }
            }
        }
    }
}

static std::shared_ptr<AstConstant> ConstantFold(std::shared_ptr<AstExpression> &left, 
    std::shared_ptr<AstExpression> &right)
{
    auto left_as_constant = std::dynamic_pointer_cast<AstConstant>(left);
    auto right_as_constant = std::dynamic_pointer_cast<AstConstant>(right);
    if (left_as_constant != nullptr && right_as_constant != nullptr) {
        // TODO
    }
    
    // one or both of the sides are not a constant
    return nullptr;
}

AstBinaryExpression::AstBinaryExpression(const std::shared_ptr<AstExpression> &left,
        const std::shared_ptr<AstExpression> &right,
        const Operator *op,
        const SourceLocation &location)
    : AstExpression(location),
      m_left(left),
      m_right(right),
      m_op(op)
{
}

void AstBinaryExpression::Visit(AstVisitor *visitor)
{
    m_left->Visit(visitor);
    m_right->Visit(visitor);

    if (m_op->ModifiesValue()) {
        auto left_as_var = std::dynamic_pointer_cast<AstVariable>(m_left);
        if (left_as_var != nullptr) {
            // make sure we are not modifying a const
            if (left_as_var->GetIdentifier() != nullptr) {
                visitor->Assert(!(left_as_var->GetIdentifier()->GetFlags() & Flag_const),
                    CompilerError(Level_fatal, Msg_const_modified, 
                        m_left->GetLocation(), left_as_var->GetName()));
            }
        }
    }
}

void AstBinaryExpression::Build(AstVisitor *visitor) const
{
    if (m_right != nullptr) {
        // the right side has not been optimized away
    }
}

void AstBinaryExpression::Optimize()
{
    OptimizeSide(m_left);
    OptimizeSide(m_right);

    // check that we can further optimize the
    // binary expression by optimizing away the right
    // side, and combining the resulting value into
    // the left side of the operation.
    auto constant_value = ConstantFold(m_left, m_right);
    if (constant_value != nullptr) {
        m_left = constant_value;
        m_right = nullptr;
        //m_op = nullptr;
    }
}

int AstBinaryExpression::IsTrue() const
{
    bool left_true = m_left->IsTrue();
    bool right_true = (m_right != nullptr) ? m_right->IsTrue() : true;

    if (left_true != -1 && right_true != -1) {
        return left_true && right_true;
    }
    // value could not be determined at compile time
    return -1;
}