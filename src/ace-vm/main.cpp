#include <iostream>
#include <fstream>

#include <ace-vm/vm.hpp>
#include <ace-vm/bytecode_stream.hpp>

#include <common/instructions.hpp>
#include <common/utf8.hpp>

#include <chrono>
#include <string>
#include <algorithm>

/** check if the option is set */
inline bool has_option(char **begin, char **end, const std::string &opt)
{
    return std::find(begin, end, opt) != end;
}

/** retrieve the value that is found after an option */
inline char *get_option_value(char **begin, char **end, const std::string &opt)
{
    char **it = std::find(begin, end, opt);
    if (it != end && ++it != end) {
        return *it;
    }
    return nullptr;
}

int main(int argc, char *argv[])
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        utf::cout << "\tUsage: " << argv[0] << " <file>\n";

    } else if (argc >= 2) {
        utf::Utf8String filename(argv[1]);

        // load bytecode from file
        std::ifstream file(filename.GetData(), std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            utf::cout << "Could not open file " << filename << "\n";
            return 1;
        }

        size_t bytecode_size = file.tellg();
        file.seekg(0, std::ios::beg);

        char *bytecodes = new char[bytecode_size];
        file.read(bytecodes, bytecode_size);
        file.close();

        BytecodeStream bytecode_stream(bytecodes, bytecode_size);

        VM vm(&bytecode_stream);
        vm.Execute();

        delete[] bytecodes;

        end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(end - start).count();
        utf::cout << "Elapsed time: " << elapsed_ms << "s\n";
    }

    return 0;
}
