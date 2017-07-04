#ifndef AST_NODE_BUILDER_HPP
#define AST_NODE_BUILDER_HPP

#include <ace-c/ast/AstFunctionDefinition.hpp>
#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/ast/AstModuleAccess.hpp>
#include <ace-c/type-system/SymbolType.hpp>

#include <common/non_owning_ptr.hpp>

#include <string>
#include <memory>
#include <vector>
#include <utility>

template <typename T>
using sp = std::shared_ptr<T>;

class AstVisitor;
class Module;
class ModuleBuilder;
class FunctionBuilder;

class AstNodeBuilder {
public:
    ModuleBuilder Module(const std::string &name);
};

class ModuleBuilder {
public:
    ModuleBuilder(
        const std::string &name
    );

    ModuleBuilder(
        const std::string &name,
        ModuleBuilder *parent
    );

    inline const std::string &GetName() const { return m_name; }

    ModuleBuilder Module(const std::string &name);
    FunctionBuilder Function(const std::string &name);

    sp<AstModuleAccess> Build(const sp<AstExpression> &expr);

private:
    std::string m_name;
    ModuleBuilder *m_parent;
};

class FunctionBuilder {
public:
    FunctionBuilder(
        const std::string &name
    );

    FunctionBuilder(
        const std::string &name,
        ModuleBuilder *parent
    );

   // FunctionBuilder *Returns(const SymbolTypePtr_t &type);
   // FunctionBuilder *Param(const std::string &name, const SymbolTypePtr_t &type);

    sp<AstExpression> Call(const std::vector<sp<AstArgument>> &args);

    inline const std::string &GetName() const { return m_name; }

private:
    std::string m_name;
    ModuleBuilder *m_parent;
};

#endif