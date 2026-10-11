#include "ConsumerApp.hpp"
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: StandaloneConsumer <diagnostic export directory>\n";
        return 2;
    }
    try {
        return ConsumerApp{}.run(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << "Consumer failure: " << error.what() << '\n';
        return 1;
    }
}
