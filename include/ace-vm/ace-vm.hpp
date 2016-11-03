#ifndef ACE_VM_HPP
#define ACE_VM_HPP

#include <ace-vm/vm.hpp>

#include <common/utf8.hpp>

namespace ace_vm {
void RunBytecodeFile(const utf::Utf8String &filename);
void RunBytecodeFile(VM *vm, const utf::Utf8String &filename, int pos = 0);
} // ace_vm

#endif
