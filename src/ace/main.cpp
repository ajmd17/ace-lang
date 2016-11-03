#include <ace-c/ace-c.hpp>
#include <ace-vm/ace-vm.hpp>

#include <common/utf8.hpp>
#include <common/str_util.hpp>

int main(int argc, char *argv[])
{
    utf::init();

    if (argc == 1) {
        utf::Utf8String temp_filename = "tmp.aex";

        int block_counter = 0;
        std::string code = "// Ace REPL\nmodule repl;\n";
        utf::cout << code;

        std::string tmp_line;
        std::string line = "";

        utf::cout << ">>> ";
        while (std::getline(utf::cin, tmp_line)) {
            for (char ch : tmp_line) {
                if (ch == '{') {
                    block_counter++;
                } else if (ch == '}') {
                    block_counter--;
                }
            }

            line += tmp_line + '\n';

            if (block_counter <= 0) {
                // send code to be compiled
                if (ace_compiler::BuildSourceString(utf::Utf8String((code + line).c_str()), temp_filename)) {
                    ace_vm::RunBytecodeFile(temp_filename);
                    code += line;
                }
                line.clear();

                utf::cout << ">>> ";
            } else {
                utf::cout << ">>> ";

                for (int i = 0; i < block_counter; i++) {
                    utf::cout << "..";
                }
            }
        }

    } else if (argc == 2) {
        utf::Utf8String src_filename = argv[1];
        utf::Utf8String out_filename = (str_util::strip_extension(argv[1]) + ".aex").c_str();

        // compile source file
        ace_compiler::BuildSourceFile(src_filename, out_filename);
        // execute the bytecode file
        ace_vm::RunBytecodeFile(out_filename);
    } else {
        utf::cout << "Invalid arguments";
    }

    return 0;
}
