#include <ace-vm/ace-vm.hpp>

#include <chrono>

int main(int argc, char *argv[])
{
    utf::init();

    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        utf::cout << "\tUsage: " << argv[0] << " <file>\n";
    } else if (argc >= 2) {
        utf::Utf8String filename(argv[1]);

        ace_vm::RunBytecodeFile(filename);

        end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<
            std::chrono::duration<double, std::ratio<1>>>(end - start).count();
        utf::cout << "Elapsed time: " << elapsed_ms << "s\n";
    }

    return 0;
}
