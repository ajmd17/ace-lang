#include <ace-c/emit/InstructionBlock.hpp>

#include <common/my_assert.hpp>

InstructionBlock::InstructionBlock()
    : m_size(0)
{
}

void InstructionBlock::Allot(Buildable *buildable)
{
    ASSERT(buildable != nullptr);

    m_size += buildable->GetSize();
    m_allotted.push_back(buildable);
}

void InstructionBlock::Build(std::streambuf &buf) const
{
    for (Buildable *buildable : m_allotted) {
        buildable->Build(buf);
    }
}