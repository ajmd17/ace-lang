#include <ace-c/emit/BytecodeUtil.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/StorageOperation.hpp>

#include <common/my_assert.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

std::unique_ptr<BytecodeChunk> BytecodeUtil::LoadSerialized(std::istream &is)
{
    std::unique_ptr<BytecodeChunk> ptr;

    cereal::JSONInputArchive iarchive(is);
    iarchive(ptr);

    return ptr;
}

void BytecodeUtil::StoreSerialized(std::ostream &os, const std::unique_ptr<BytecodeChunk> &chunk)
{
    cereal::JSONOutputArchive oarchive(os);
    oarchive(chunk);
}