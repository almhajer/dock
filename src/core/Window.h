#pragma once

#include <string>
#include <cstdint>

struct GLFWwindow;

namespace core {

/// نافذة اللعبة - تغليف لنافذة GLFW مع إدارة دورة الحياة
class Window {
public:
    /// إعدادات النافذة
    struct Config {
        std::string title;                 // عنوان النافذة
        uint32_t width;                    // العرض بالبكسل
        uint32_t height;                   // الارتفاع بالبكسل
        bool fullscreen;                   // ملء الشاشة
        bool resizable;                    // قابلة لتغيير الحجم
        bool vsync;                        // مزامنة عمودية

        Config()
            : title("DuckH")
            , width(1280)
            , height(720)
            , fullscreen(false)
            , resizable(true)
            , vsync(true) {}
    };

    explicit Window(const Config& config = {});
    ~Window();

    // منع النسخ لأن النافذة مورد وحيد
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// هل النافذة مفتوحة ولم يُطلب إغلاقها؟
    [[nodiscard]] bool isOpen() const;

    /// معالجة أحداث النافذة (إدخال، تغيير حجم...)
    void pollEvents();

    /// انتظار حتى يحين وقت الإطار التالي (إذا كان VSync مفعّلًا)
    void waitEvents();

    /// الحصول على مقبض GLFW الخام (للاستخدام مع Vulkan)
    [[nodiscard]] GLFWwindow* getHandle() const;

    /// أبعاد النافذة الحالية
    [[nodiscard]] uint32_t getWidth() const;
    [[nodiscard]] uint32_t getHeight() const;

    /// هل تغيّر حجم النافذة منذ آخر فحص؟ (يُعاد ضبطه بعد القراءة)
    [[nodiscard]] bool wasResized();

    /// تعيين VSync
    void setVSync(bool enabled);

private:
    /// رد نداء تغيير حجم النافذة
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* mHandle = nullptr;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    bool mResized = false;
};

} // namespace core
