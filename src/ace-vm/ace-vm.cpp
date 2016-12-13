#include <ace-vm/ace-vm.hpp>
#include <ace-vm/bytecode_stream.hpp>

#include <common/my_assert.hpp>

#include <fstream>
#include <chrono>
#include <cstdint>

namespace ace_vm {

void RunBytecodeFile(const utf::Utf8String &filename)
{
    VM vm;
    RunBytecodeFile(&vm, filename);
}

void RunBytecodeFile(VM *vm, const utf::Utf8String &filename, bool record_time, int pos)
{
    ASSERT(vm != nullptr);

    // load bytecode from file
    std::ifstream file(filename.GetData(),
        std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        utf::cout << "Could not open file " << filename << "\n";
        return;
    }

    int64_t bytecode_size = file.tellg();
    file.seekg(0, std::ios::beg);

    ASSERT(bytecode_size > 0);

    char *bytecodes = new char[bytecode_size];
    file.read(bytecodes, bytecode_size);
    file.close();

    BytecodeStream bytecode_stream(non_owning_ptr<char>(bytecodes), (size_t)bytecode_size, pos);

    vm->GetState().SetBytecodeStream(non_owning_ptr<BytecodeStream>(&bytecode_stream));

    // time how long execution took
    auto start = std::chrono::high_resolution_clock::now();

    vm->Execute();

    if (record_time) {
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<
                std::chrono::duration<double, std::ratio<1>>>(end - start).count();
        utf::cout << "Elapsed time: " << elapsed_ms << "s\n";
    }

    delete[] bytecodes;
}

} // ace_vm
