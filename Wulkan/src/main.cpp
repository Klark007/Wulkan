#include <iostream>
#include "engine/Engine.h"

// initial screen resolution
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() {
    Engine app {};

    try {
        app.init(WIDTH, HEIGHT);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        std::cout << "Press anything to close:";
        char c;
        std::cin >> c;
        
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}