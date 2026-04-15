/**
 * @file Window.h
 * @brief واجهة إدارة النافذة.
 * @details تغليف لنافذة GLFW مع إدارة دورة الحياة.
 */

#pragma once

#include <string>
#include <cstdint>

struct GLFWwindow;

namespace core
{

/**
 * @brief نافذة اللعبة - تغليف لنافذة GLFW مع إدارة دورة الحياة.
 */
class Window
{
  public:
#pragma region PublicTypes
    /**
     * @brief إعدادات النافذة.
     */
    struct Config
    {
        /**
         * @brief عنوان النافذة الظاهر للمستخدم.
         */
        std::string title; // عنوان النافذة

        /**
         * @brief العرض المطلوب عند الإنشاء.
         */
        uint32_t width; // العرض بالبكسل

        /**
         * @brief الارتفاع المطلوب عند الإنشاء.
         */
        uint32_t height; // الارتفاع بالبكسل

        /*
         هل يبدأ التطبيق بملء الشاشة؟
         */
        bool fullscreen; // ملء الشاشة

        /**
         * @brief هل يسمح بتغيير الحجم؟
         */
        bool resizable; // قابلة لتغيير الحجم

        /**
         * @brief هل تُفعّل المزامنة العمودية؟
         */
        bool vsync; // مزامنة عمودية

        Config() : title("DuckH"), width(1600), height(900), fullscreen(false), resizable(true), vsync(true)
        {
        }
    };
#pragma endregion PublicTypes

    /**
     * @brief ينشئ النافذة ويهيئ GLFW حسب الإعدادات.
     * @param config إعدادات النافذة.
     * @throws std::runtime_error إذا فشل إنشاء النافذة.
     */
    explicit Window(const Config& config = {});

    /**
     * @brief يغلق النافذة ويحرر موارد GLFW المرتبطة بها.
     */
    ~Window();

    // منع النسخ لأن النافذة مورد وحيد
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /**
     * @brief هل النافذة مفتوحة ولم يُطلب إغلاقها؟
     * @return true إذا كانت النافذة صالحة، false خلاف ذلك.
     */
    [[nodiscard]] bool isOpen() const;

    /**
     * @brief معالجة أحداث النافذة (إدخال، تغيير حجم...).
     */
    void pollEvents();

    /**
     * @brief تعيين أيقونة نافذة GLFW من ملف صورة.
     * @param iconPath مسار ملف الأيقونة.
     */
    void setIcon(const std::string& iconPath);

    /**
     * @brief انتظار حتى يحين وقت الإطار التالي (إذا كان VSync مفعّلًا).
     */
    void waitEvents();

    /**
     * @brief الحصول على مقبض GLFW الخام (للاستخدام مع Vulkan).
     * @return مؤشر لنافذة GLFW.
     */
    [[nodiscard]] GLFWwindow* getHandle() const;

    /**
     * @brief أبعاد النافذة الحالية.
     * @return العرض بالبكسل.
     */
    [[nodiscard]] uint32_t getWidth() const;
    /**
     * @brief أبعاد النافذة الحالية.
     * @return الارتفاع بالبكسل.
     */
    [[nodiscard]] uint32_t getHeight() const;

    /**
     * @brief هل تغيّر حجم النافذة منذ آخر فحص؟ (يُعاد ضبطه بعد القراءة).
     * @return true إذا تغير الحجم، false خلاف ذلك.
     */
    [[nodiscard]] bool wasResized();

    /*
     تعيين VSync
     */
    void setVSync(bool enabled);

    /*
     إظهار/إخفاء مؤشر الماوس
     */
    void setCursorVisible(bool visible);

    /*
     هل المؤشر مرئي؟
     */
    [[nodiscard]] bool isCursorVisible() const;

  private:
#pragma region InternalCallbacks
    /*
     رد نداء تغيير حجم النافذة
     */
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
#pragma endregion InternalCallbacks

#pragma region InternalState
    /*
     مقبض نافذة GLFW الخام.
     */
    GLFWwindow* mHandle = nullptr;

    /*
     العرض الحالي للنافذة.
     */
    uint32_t mWidth = 0;

    /*
     الارتفاع الحالي للنافذة.
     */
    uint32_t mHeight = 0;

    /*
     هل تم تغيير الحجم منذ آخر فحص؟
     */
    bool mResized = false;

    /*
     هل المؤشر ظاهر للمستخدم؟
     */
    bool mCursorVisible = true;
#pragma endregion InternalState
};

} // namespace core
