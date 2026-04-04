#include "core/App.h"

#include <filesystem>
#include <iostream>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#endif

namespace {

#ifdef __APPLE__
std::string bundleResourcesPath()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle == nullptr)
    {
        return {};
    }

    CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(bundle);
    if (resourcesUrl == nullptr)
    {
        return {};
    }

    char path[PATH_MAX] = {};
    const bool success = CFURLGetFileSystemRepresentation(
        resourcesUrl,
        true,
        reinterpret_cast<UInt8*>(path),
        sizeof(path));
    CFRelease(resourcesUrl);

    return success ? std::string(path) : std::string{};
}
#endif

void prepareWorkingDirectory()
{
#ifdef __APPLE__
    const std::string resourcesPath = bundleResourcesPath();
    if (resourcesPath.empty())
    {
        return;
    }

    std::error_code error;
    std::filesystem::current_path(resourcesPath, error);
    if (error)
    {
        std::cerr << "[Init] تعذر تغيير مجلد العمل إلى Resources: "
                  << error.message() << std::endl;
    }
#endif
}

} // namespace

int main() {
    try {
        prepareWorkingDirectory();

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
