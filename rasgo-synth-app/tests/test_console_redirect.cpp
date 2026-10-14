#include "../src/ConsoleRedirect.h"
#include <cstdio>
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    if (!rasgo::diagnostics::redirectConsole(std::filesystem::u8path(argv[1]))) return 3;
    std::puts("studio stdout");
    std::fputs("studio stderr\n", stderr);
    return 0;
}
