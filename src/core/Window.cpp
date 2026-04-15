/**
 * @file Window.cpp
 * @brief تنفيذ النافذة.
 * @details إدارة النافذة باستخدام GLFW.
 */

#include "Window.h"

#include <GLFW/glfw3.h>
#include "../gfx/stb_image.h"
#include <iostream>
#include <stdexcept>

#ifdef __APPLE__
#include <CoreGraphics/CGSession.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace core
{

namespace
{

#ifdef __APPLE__
[[nodiscard]] bool sessionFlagIsTrue(CFDictionaryRef sessionInfo, const void* key)
{
    if (sessionInfo == nullptr || key == nullptr)
    {
        return false;
    }

    const void* value = CFDictionaryGetValue(sessionInfo, key);
    return value == kCFBooleanTrue;
}

[[nodiscard]] bool hasGuiSession()
{
    CFDictionaryRef sessionInfo = CGSessionCopyCurrentDictionary();
    if (sessionInfo == nullptr)
    {
        return false;
    }

    const bool onConsole = sessionFlagIsTrue(sessionInfo, kCGSessionOnConsoleKey);
    const bool loginDone = sessionFlagIsTrue(sessionInfo, kCGSessionLoginDoneKey);
    CFRelease(sessionInfo);
    return onConsole && loginDone;
}
#endif

} // namespace

Window::Window(const Config& config)
{
#ifdef __APPLE__
    if (!hasGuiSession())
    {
        throw std::runtime_error("يتطلب تشغيل DuckH جلسة macOS رسومية نشطة. شغّل التطبيق من Finder أو عبر open، وليس من "
                                 "SSH أو sudo أو خدمة خلفية.");
    }
#endif

    // تهيئة GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("فشل تهيئة GLFW");
    }

    // Vulkan لا يحتاج سياق OpenGL - نعطّل إنشاء السياق التلقائي
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // إنشاء النافذة
    mHandle = glfwCreateWindow(static_cast<int>(config.width), static_cast<int>(config.height), config.title.c_str(),
                               config.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);

    if (!mHandle)
    {
        glfwTerminate();
        throw std::runtime_error("فشل إنشاء نافذة GLFW");
    }

    mWidth = config.width;
    mHeight = config.height;

    // ربط رد نداء تغيير حجم الإطار
    glfwSetWindowUserPointer(mHandle, this);
    glfwSetFramebufferSizeCallback(mHandle, framebufferResizeCallback);

    // تطبيق VSync (يتطلب سياق OpenGL، لكن نسجله كعلم للاستخدام لاحقًا)
    setVSync(config.vsync);
}

Window::~Window()
{
    if (mHandle)
    {
        glfwDestroyWindow(mHandle);
    }
    glfwTerminate();
}

bool Window::isOpen() const
{
    return mHandle && !glfwWindowShouldClose(mHandle);
}

void Window::pollEvents()
{
    glfwPollEvents();

    if (!mHandle)
    {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mHandle, &fbWidth, &fbHeight);

    const uint32_t newWidth = static_cast<uint32_t>(fbWidth);
    const uint32_t newHeight = static_cast<uint32_t>(fbHeight);
    if (newWidth != mWidth || newHeight != mHeight)
    {
        mWidth = newWidth;
        mHeight = newHeight;
        mResized = true;
    }
}

void Window::setIcon(const std::string& iconPath)
{
#ifdef __APPLE__
    /*
     * macOS يتجاهل glfwSetWindowIcon، لذلك تُدار أيقونة الـ Dock عبر DockIcon.mm.
     */
    (void)iconPath;
    return;
#else
    if (!mHandle)
    {
        return;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr || width <= 0 || height <= 0)
    {
        std::cerr << "[Window] تعذر تحميل أيقونة النافذة: " << iconPath;
        if (const char* reason = stbi_failure_reason(); reason != nullptr)
        {
            std::cerr << " (" << reason << ")";
        }
        std::cerr << std::endl;
        return;
    }

    GLFWimage image{};
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    glfwSetWindowIcon(mHandle, 1, &image);

#ifdef _WIN32
    /*
     * على ويندوز قد لا يتحدث شريط المهام فورًا إلا بعد ضخ دورة أحداث واحدة.
     */
    glfwPollEvents();
#endif

    stbi_image_free(pixels);
    std::cout << "[Window] تم ضبط أيقونة النافذة" << std::endl;
#endif
}

void Window::waitEvents()
{
    glfwWaitEvents();
}

GLFWwindow* Window::getHandle() const
{
    return mHandle;
}

uint32_t Window::getWidth() const
{
    return mWidth;
}

uint32_t Window::getHeight() const
{
    return mHeight;
}

bool Window::wasResized()
{
    bool result = mResized;
    mResized = false; // إعادة ضبط بعد القراءة
    return result;
}

void Window::setVSync(bool /*enabled*/)
{
    // ملاحظة: VSync في Vulkan يُدار عبر Swapchain (VK_PRESENT_MODE_FIFO_KHR)
    // هذا العلم يُستخدم لاحقًا عند إنشاء/إعادة إنشاء Swapchain
}

void Window::setCursorVisible(bool visible)
{
    if (!mHandle)
        return;
    mCursorVisible = visible;
    glfwSetInputMode(mHandle, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

bool Window::isCursorVisible() const
{
    return mCursorVisible;
}

// ─── ردود نداء GLFW ────────────────────────────────────────────

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!win)
        return;

    win->mWidth = static_cast<uint32_t>(width);
    win->mHeight = static_cast<uint32_t>(height);
    win->mResized = true;
}

} // namespace core
