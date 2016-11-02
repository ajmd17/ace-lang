#include <ace-c/ace-c.hpp>
#include <common/cli_args.hpp>

int main(int argc, char *argv[])
{
    utf::init();

    if (argc == 1) {
        utf::cout << "\tUsage: " << argv[0] << " <file>\n";
    } else if (argc >= 2) {
        enum {
            COMPILE_SOURCE,
            DECOMPILE_BYTECODE,
        } mode = COMPILE_SOURCE;

        utf::Utf8String filename;
        utf::Utf8String out_filename;

        if (CLI::HasOption(argv, argv + argc, "-d")) {
            // disassembly mode
            mode = DECOMPILE_BYTECODE;
            filename = CLI::GetOptionValue(argv, argv + argc, "-d");

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }
        } else {
            mode = COMPILE_SOURCE;

            if (CLI::HasOption(argv, argv + argc, "-c")) {
                filename = CLI::GetOptionValue(argv, argv + argc, "-c");
            }

            if (filename == "") {
                filename = argv[1];
            }

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }

            if (out_filename == "") {
                out_filename = "out.bin";
            }
        }

        if (mode == COMPILE_SOURCE) {
            ace_compiler::BuildSourceFile(filename, out_filename);
        } else if (mode == DECOMPILE_BYTECODE) {
            ace_compiler::DecompileBytecodeFile(filename, out_filename);
        }
    }

    return 0;
}
