#pragma once

#include <unordered_map>
#include <cstdint>

struct GLFWwindow;

namespace core {

/// مدير الإدخال - يتتبع حالة لوحة المفاتيح والماوس
class Input {
public:
    #pragma region PublicInterface
    /// ينشئ مدير الإدخال بحالته الافتراضية.
    Input() = default;

    /// تهيئة الاستماع لأحداث النافذة
    void init(GLFWwindow* window);

    /// تحديث حالة الإدخال - يُستدعى بداية كل إطار
    void update();

    /// قطع الاتصال مع GLFW وتنظيف البيانات
    void shutdown();

    /// هل المفتاح مضغوط حاليًا؟
    [[nodiscard]] bool isKeyPressed(int key) const;

    /// هل المفتاح ضُغط في هذا الإطار فقط (لحظي)؟
    [[nodiscard]] bool isKeyJustPressed(int key) const;

    /// هل زر الماوس مضغوط؟ (0=أيسر، 1=أيمن، 2=أوسط)
    [[nodiscard]] bool isMouseButtonPressed(int button) const;

    /// هل زر الماوس ضُغط في هذا الإطار فقط؟
    [[nodiscard]] bool isMouseButtonJustPressed(int button) const;

    /// موقع المؤشر بالنسبة للنافذة (بالبكسل)
    [[nodiscard]] float getMouseX() const;
    [[nodiscard]] float getMouseY() const;

    /// إزاحة المؤشر منذ آخر إطار
    [[nodiscard]] float getMouseDeltaX() const;
    [[nodiscard]] float getMouseDeltaY() const;
    #pragma endregion PublicInterface

private:
    #pragma region CallbackBridge
    /// ردود نداء GLFW (friends للوصول للبيانات الخاصة)
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double cursorX, double cursorY);
    #pragma endregion CallbackBridge

    #pragma region InternalState
    /// مقبض نافذة GLFW المرتبط بالإدخال.
    GLFWwindow* mWindow = nullptr;

    // حالة المفاتيح: current = الإطار الحالي، previous = الإطار السابق

    /// حالة المفاتيح في الإطار الحالي.
    std::unordered_map<int, bool> mKeysCurrent;

    /// حالة المفاتيح في الإطار السابق.
    std::unordered_map<int, bool> mKeysPrevious;

    // حالة أزرار الماوس

    /// حالة أزرار الماوس في الإطار الحالي.
    std::unordered_map<int, bool> mMouseButtonsCurrent;

    /// حالة أزرار الماوس في الإطار السابق.
    std::unordered_map<int, bool> mMouseButtonsPrevious;

    // موقع المؤشر

    /// موقع المؤشر الأفقي الحالي.
    float mMouseX = 0.0f;

    /// موقع المؤشر العمودي الحالي.
    float mMouseY = 0.0f;

    /// آخر موقع أفقي معروف للمؤشر.
    float mMouseLastX = 0.0f;

    /// آخر موقع عمودي معروف للمؤشر.
    float mMouseLastY = 0.0f;

    /// فرق الحركة الأفقي للمؤشر منذ آخر إطار.
    float mMouseDeltaX = 0.0f;

    /// فرق الحركة العمودي للمؤشر منذ آخر إطار.
    float mMouseDeltaY = 0.0f;
    #pragma endregion InternalState
};

} // namespace core
