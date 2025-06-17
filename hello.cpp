#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::cout << "Hello\n";

    std::vector<std::string> args;
    for (int i=0; i<argc; i++) {
        args.emplace_back(argv[i]);
    }

    for (auto &arg: args) {
        std::cout << "Arg: " << arg << "\n";
    }

    return 0;
}
