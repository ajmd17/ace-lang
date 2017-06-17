#ifndef INSTRUCTION_BLOCK_HPP
#define INSTRUCTION_BLOCK_HPP

#include <ace-c/emit/InstructionObject.hpp>

#include <vector>
#include <streambuf>

class InstructionBlock {
public:
    InstructionBlock();

    inline size_t GetCurrentSize() const
        { return m_size; }

    void Allot(Buildable *buildable);
    void Build(std::streambuf &buf) const;

private:
    std::vector<Buildable*> m_allotted;
    size_t m_size;
};

#endif