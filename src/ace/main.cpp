#include <ace-c/ace-c.hpp>
#include <ace-vm/ace-vm.hpp>

#include <common/utf8.hpp>
#include <common/str_util.hpp>

int main(int argc, char *argv[])
{
    utf::init();

    if (argc == 1) {
        utf::cout << "acelang 0.2 REPL \n";
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
