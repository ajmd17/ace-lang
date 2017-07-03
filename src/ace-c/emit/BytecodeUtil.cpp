#include <ace-c/emit/BytecodeUtil.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/StorageOperation.hpp>
#include <ace-c/emit/aex/AEXGenerator.hpp>

#include <common/my_assert.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

std::vector<std::uint8_t> BytecodeUtil::GenerateBytes(BytecodeChunk *chunk)
{
    BuildParams build_params;
    return GenerateBytes(chunk, build_params);
}

std::vector<std::uint8_t> BytecodeUtil::GenerateBytes(BytecodeChunk *chunk, BuildParams &build_params)
{
    ASSERT(chunk != nullptr);

    std::basic_stringbuf<std::uint8_t> buf;

    AEXGenerator aex_gen(buf, build_params);
    aex_gen.Visit(chunk);

    //chunk->Build(buf, build_params);

    std::vector<std::uint8_t> vec;
    vec.reserve(chunk->GetSize());
    vec.insert(
        vec.end(),
        std::istreambuf_iterator<std::uint8_t>(&buf),
        std::istreambuf_iterator<std::uint8_t>()
    );
    
    return vec;
}

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