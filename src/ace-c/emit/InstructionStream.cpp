#include <ace-c/emit/InstructionStream.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <cstring>
#include <algorithm>
#include <iostream>
#include <sstream>

InstructionStream::InstructionStream()
    : m_register_counter(0),
      m_stack_size(0),
      m_static_id(0)
{
}

InstructionStream::InstructionStream(const InstructionStream &other)
    : m_register_counter(other.m_register_counter),
      m_stack_size(other.m_stack_size),
      m_static_id(other.m_static_id),
      m_static_objects(other.m_static_objects)
{
}

int InstructionStream::FindStaticObject(const StaticObject &static_object) const
{
    for (const StaticObject &so : m_static_objects) {
        if (so == static_object) {
            return so.m_id;
        }
    }
    // not found
    return -1;
}