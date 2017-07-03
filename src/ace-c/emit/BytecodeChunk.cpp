#include <ace-c/emit/BytecodeChunk.hpp>

#include <iostream>

BytecodeChunk::BytecodeChunk()
    : chunk_size(0)
{
}

void BytecodeChunk::Build(Buffer &buf, BuildParams &build_params) const
{
    BuildParams new_params;
    new_params.block_offset = build_params.block_offset + build_params.local_offset;
    new_params.local_offset = 0;
    new_params.labels = labels;

    for (auto &buildable : buildables) {
        buildable->Build(buf, new_params);
    }

    build_params.local_offset += chunk_size;
}