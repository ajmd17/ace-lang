#include <ace-vm/ace-vm.hpp>

int main(int argc, char *argv[])
{
    utf::init();

    if (argc == 1) {
        utf::cout << "\tUsage: " << argv[0] << " <file>\n";
    } else if (argc >= 2) {
        ace_vm::RunBytecodeFile(argv[1]);
    }

    return 0;
}
