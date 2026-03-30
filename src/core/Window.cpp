#include "Window.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace core {

Window::Window(const Config& config) {
    // تهيئة GLFW
    if (!glfwInit()) {
        throw std::runtime_error("فشل تهيئة GLFW");
    }

    // Vulkan لا يحتاج سياق OpenGL - نعطّل إنشاء السياق التلقائي
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // إنشاء النافذة
    mHandle = glfwCreateWindow(
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.title.c_str(),
        config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr
    );

    if (!mHandle) {
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

Window::~Window() {
    if (mHandle) {
        glfwDestroyWindow(mHandle);
    }
    glfwTerminate();
}

bool Window::isOpen() const {
    return mHandle && !glfwWindowShouldClose(mHandle);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::waitEvents() {
    glfwWaitEvents();
}

GLFWwindow* Window::getHandle() const {
    return mHandle;
}

uint32_t Window::getWidth() const {
    return mWidth;
}

uint32_t Window::getHeight() const {
    return mHeight;
}

bool Window::wasResized() {
    bool result = mResized;
    mResized = false; // إعادة ضبط بعد القراءة
    return result;
}

void Window::setVSync(bool /*enabled*/) {
    // ملاحظة: VSync في Vulkan يُدار عبر Swapchain (VK_PRESENT_MODE_FIFO_KHR)
    // هذا العلم يُستخدم لاحقًا عند إنشاء/إعادة إنشاء Swapchain
}

// ─── ردود نداء GLFW ────────────────────────────────────────────

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!win) return;

    win->mWidth = static_cast<uint32_t>(width);
    win->mHeight = static_cast<uint32_t>(height);
    win->mResized = true;
}

} // namespace core
