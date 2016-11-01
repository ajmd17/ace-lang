#ifndef TREE_HPP
#define TREE_HPP

#include <vector>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <stack>
#include <string>

template <typename T>
struct TreeNode {
    TreeNode(const T &value)
        : m_value(value)
    {
    }

    ~TreeNode()
    {
        for (int i = 0; i < m_children.size(); i++) {
            if (m_children[i] != nullptr) {
                delete m_children[i];
                m_children[i] = nullptr;
            }
        }
    }

    void PrintToStream(std::stringstream &ss, int &indent_level) const
    {
        for (int i = 0; i < indent_level; i++) {
            ss << "  ";
        }
        ss << m_value << "\n";

        indent_level++;
        for (int i = 0; i < m_children.size(); i++) {
            m_children[i]->PrintToStream(ss, indent_level);
        }
        indent_level--;
    }

    TreeNode<T> *m_parent = nullptr;
    std::vector<TreeNode<T>*> m_children;
    T m_value;
};

template <typename T>
class Tree {
public:
    std::ostream &operator<<(std::ostream &os) const
    {
        std::stringstream ss;
        int indent_level = 0;

        for (int i = 0; i < m_nodes.size(); i++) {
            m_nodes[i]->PrintToStream(ss, indent_level);
        }

        os << ss.str() << "\n";

        return os;
    }

public:
    Tree()
        : m_top(nullptr)
    {
        // open root
        Open(T());
    }

    ~Tree()
    {
        // close root
        Close();

        // first in, first out
        for (int i = m_nodes.size() - 1; i >= 0; i--) {
            if (m_nodes[i] != nullptr) {
                delete m_nodes[i];
            }
        }
    }

    inline TreeNode<T> *TopNode() {
        return m_top;
    }

    inline const TreeNode<T> *TopNode() const {
        return m_top;
    }

    inline T &Top()
    {
        if (!m_top) {
            throw std::runtime_error("no top value");
        }
        return m_top->m_value;
    }

    inline const T &Top() const
    {
        if (!m_top) {
            throw std::runtime_error("no top value");
        }
        return m_top->m_value;
    }

    void Open(const T &value)
    {
        TreeNode<T> *node = new TreeNode<T>(value);
        node->m_parent = m_top;

        if (m_top != nullptr) {
            m_top->m_children.push_back(node);
        } else {
            m_nodes.push_back(node);
        }

        m_top = node;
    }

    void Close()
    {
        if (m_top == nullptr) {
            throw std::runtime_error("already closed!");
        }

        m_top = m_top->m_parent;
    }

private:
    std::vector<TreeNode<T>*> m_nodes;
    TreeNode<T> *m_top;
};

#endif
