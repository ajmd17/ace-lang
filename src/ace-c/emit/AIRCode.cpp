#include <ace-c/emit/AIRCode.hpp>

#include <cereal/archives/json.hpp>

AIRCodeChunk::AIRCodeChunk()
    : AIRBuildable(AIR_TYPE_CHUNK),
      chunk_size(0)
{
}

/*AIRCodeChunk::AIRCodeChunk(const std::vector<AIRLabel> &labels, size_t chunk_size, const std::vector<std::unique_ptr<AIRBuildable>> &buildables)
    : labels(labels),
      chunk_size(chunk_size),
      buildables(buildables)
{
}*/

void AIRCodeChunk::Build(Buffer &buf, AIRParams &params) const
{
    AIRParams new_params;
    new_params.block_offset = params.block_offset + params.local_offset;
    new_params.local_offset = 0;
    new_params.labels = labels;

    for (auto &buildable : buildables) {
        buildable->Build(buf, new_params);
    }

    params.local_offset += chunk_size;
}

void AIR::Store(std::ostream &os, const std::unique_ptr<AIRCodeChunk> &chunk)
{
    cereal::JSONOutputArchive oarchive(os);
    oarchive(chunk);
}

std::unique_ptr<AIRCodeChunk> AIR::Load(std::istream &is)
{
    std::unique_ptr<AIRCodeChunk> ptr;

    cereal::JSONInputArchive iarchive(is);
    iarchive(ptr);

    return ptr;
}
