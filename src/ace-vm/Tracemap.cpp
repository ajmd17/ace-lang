#include <ace-vm/Tracemap.hpp>

namespace ace {
namespace vm {

void Tracemap::Set(StringmapEntry *stringmap, LinemapEntry *linemap)
{
    m_stringmap = stringmap;
    m_linemap = linemap;
}

} // namespace vm
} // namespace ace
