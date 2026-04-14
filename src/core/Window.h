#pragma once

#include <string>
#include <cstdint>

struct GLFWwindow;

namespace core
{

/*
 نافذة اللعبة - تغليف لنافذة GLFW مع إدارة دورة الحياة
 */
class Window
{
  public:
#pragma region PublicTypes
    /*
     إعدادات النافذة
     */
    struct Config
    {
        /*
         عنوان النافذة الظاهر للمستخدم.
         */
        std::string title; // عنوان النافذة

        /*
         العرض المطلوب عند الإنشاء.
         */
        uint32_t width; // العرض بالبكسل

        /*
         الارتفاع المطلوب عند الإنشاء.
         */
        uint32_t height; // الارتفاع بالبكسل

        /*
         هل يبدأ التطبيق بملء الشاشة؟
         */
        bool fullscreen; // ملء الشاشة

        /*
         هل يسمح بتغيير الحجم؟
         */
        bool resizable; // قابلة لتغيير الحجم

        /*
         هل تُفعّل المزامنة العمودية؟
         */
        bool vsync; // مزامنة عمودية

        Config() : title("DuckH"), width(1600), height(900), fullscreen(false), resizable(true), vsync(true)
        {
        }
    };
#pragma endregion PublicTypes

    /*
     ينشئ النافذة ويهيئ GLFW حسب الإعدادات.
     */
    explicit Window(const Config& config = {});

    /*
     يغلق النافذة ويحرر موارد GLFW المرتبطة بها.
     */
    ~Window();

    // منع النسخ لأن النافذة مورد وحيد
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /*
     هل النافذة مفتوحة ولم يُطلب إغلاقها؟
     */
    [[nodiscard]] bool isOpen() const;

    /*
     معالجة أحداث النافذة (إدخال، تغيير حجم...)
     */
    void pollEvents();

    /*
     تعيين أيقونة نافذة GLFW من ملف صورة.
     */
    void setIcon(const std::string& iconPath);

    /*
     انتظار حتى يحين وقت الإطار التالي (إذا كان VSync مفعّلًا)
     */
    void waitEvents();

    /*
     الحصول على مقبض GLFW الخام (للاستخدام مع Vulkan)
     */
    [[nodiscard]] GLFWwindow* getHandle() const;

    /*
     أبعاد النافذة الحالية
     */
    [[nodiscard]] uint32_t getWidth() const;
    [[nodiscard]] uint32_t getHeight() const;

    /*
     هل تغيّر حجم النافذة منذ آخر فحص؟ (يُعاد ضبطه بعد القراءة)
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
