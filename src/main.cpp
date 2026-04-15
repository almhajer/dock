#include "core/App.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#endif

namespace
{

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
    const bool success =
        CFURLGetFileSystemRepresentation(resourcesUrl, true, reinterpret_cast<UInt8*>(path), sizeof(path));
    CFRelease(resourcesUrl);

    return success ? std::string(path) : std::string{};
}

std::filesystem::path logFilePath()
{
    const char* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
    {
        return {};
    }

    return std::filesystem::path(home) / "Library" / "Logs" / "Duck.log";
}

void appendLogLine(const std::string& message)
{
    const std::filesystem::path logPath = logFilePath();
    if (logPath.empty())
    {
        return;
    }

    std::error_code error;
    std::filesystem::create_directories(logPath.parent_path(), error);

    std::ofstream output(logPath, std::ios::app);
    if (output)
    {
        output << message << '\n';
    }
}

void configureBundledVulkanDriver()
{
    const std::string resourcesPath = bundleResourcesPath();
    if (resourcesPath.empty())
    {
        return;
    }

    const std::filesystem::path icdPath = std::filesystem::path(resourcesPath) / "vulkan" / "icd.d" / "MoltenVK_icd.json";
    if (!std::filesystem::exists(icdPath))
    {
        appendLogLine("[Init] Bundled MoltenVK ICD was not found at: " + icdPath.string());
        return;
    }

    setenv("VK_DRIVER_FILES", icdPath.c_str(), 1);
    setenv("VK_ICD_FILENAMES", icdPath.c_str(), 1);
    appendLogLine("[Init] Using bundled MoltenVK ICD: " + icdPath.string());
}
#else
void appendLogLine(const std::string&)
{
}

void configureBundledVulkanDriver()
{
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
        std::cerr << "[Init] تعذر تغيير مجلد العمل إلى Resources: " << error.message() << std::endl;
    }
#endif
}

} // namespace

int main()
{
    try
    {
        appendLogLine("[Init] Launching Duck");
        configureBundledVulkanDriver();
        prepareWorkingDirectory();

        core::App::Config config;
        config.window.title = "Duck - صيد البط";
        config.window.width = 1600;
        config.window.height = 900;

        core::App app(config);
        app.run();
    }
    catch (const std::exception& e)
    {
        appendLogLine(std::string("[Fatal] ") + e.what());
        std::cerr << "خطأ فادح: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
