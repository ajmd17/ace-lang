#ifndef ACE_VM_HPP
#define ACE_VM_HPP

#include <common/utf8.hpp>

namespace ace_vm {
void RunBytecodeFile(const utf::Utf8String &filename);
} // ace_vm

#endif
