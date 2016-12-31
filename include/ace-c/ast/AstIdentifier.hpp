#ifndef AST_IDENTIFIER_HPP
#define AST_IDENTIFIER_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/Identifier.hpp>
#include <ace-c/enums.hpp>

#include <string>

struct AstIdentifierProperties {
    Identifier *m_identifier = nullptr;

    AccessMode m_access_mode = ACCESS_MODE_LOAD;
    IdentifierType m_identifier_type = IDENTIFIER_TYPE_UNKNOWN;

    bool m_is_in_function = false;
    int m_depth = 0;

	// getters & setters
	inline Identifier *GetIdentifier() { return m_identifier; }
	inline const Identifier *GetIdentifier() const { return m_identifier; }
	inline void SetIdentifier(Identifier *identifier) { m_identifier = identifier; }

	inline AccessMode GetAccessMode() const { return m_access_mode; }
	inline void SetAccessMode(AccessMode access_mode) { m_access_mode = access_mode; }

	inline IdentifierType GetIdentifierType() const { return m_identifier_type; }
	inline void SetIdentifierType(IdentifierType identifier_type) { m_identifier_type = identifier_type; }

	inline bool IsInFunction() const { return m_is_in_function; }
	inline int GetDepth() const { return m_depth; }
};

class AstIdentifier : public AstExpression {
public:
    AstIdentifier(const std::string &name, const SourceLocation &location);
    virtual ~AstIdentifier() = default;

    void PerformLookup(AstVisitor *visitor, Module *mod);
    void CheckInFunction(AstVisitor *visitor, Module *mod);

    inline const std::string &GetName() const { return m_name; }
    inline AstIdentifierProperties &GetProperties() { return m_properties; }
    inline const AstIdentifierProperties &GetProperties() const { return m_properties; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Recreate(std::ostringstream &ss) override = 0;

    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override = 0;
    virtual SymbolTypePtr_t GetSymbolType() const override;

protected:
    std::string m_name;
    
    AstIdentifierProperties m_properties;

    int GetStackOffset(int stack_size) const;
};

#endif
