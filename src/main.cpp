#include "core/App.h"

#include <iostream>

int main() {
    try {
        core::App::Config config;
        config.window.title = "DuckH - صيد البط";
        config.window.width = 1600;
        config.window.height = 900;

        core::App app(config);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "خطأ فادح: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
