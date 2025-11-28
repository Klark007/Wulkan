#include "common.h"
#include "Engine.h"

#include <iostream>

#define SPDLOG_FMT_EXTERNAL // Use already existing fmt implementation (should already be definied in common.h)
#include "spdlog/spdlog.h"

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
        spdlog::error(e.what());
        
        spdlog::info("Press anything to close:");
        spdlog::shutdown();
        
        char c;
        std::cin >> c;
        
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}