#pragma once

#include <unordered_map>
#include <cstdint>

struct GLFWwindow;

namespace core {

/// مدير الإدخال - يتتبع حالة لوحة المفاتيح والماوس
class Input {
public:
    Input() = default;

    /// تهيئة الاستماع لأحداث النافذة
    void init(GLFWwindow* window);

    /// تحديث حالة الإدخال - يُستدعى بداية كل إطار
    void update();

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

private:
    /// ردود نداء GLFW (friends للوصول للبيانات الخاصة)
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* mWindow = nullptr;

    // حالة المفاتيح: current = الإطار الحالي، previous = الإطار السابق
    std::unordered_map<int, bool> mKeysCurrent;
    std::unordered_map<int, bool> mKeysPrevious;

    // حالة أزرار الماوس
    std::unordered_map<int, bool> mMouseButtonsCurrent;
    std::unordered_map<int, bool> mMouseButtonsPrevious;

    // موقع المؤشر
    float mMouseX = 0.0f;
    float mMouseY = 0.0f;
    float mMouseLastX = 0.0f;
    float mMouseLastY = 0.0f;
    float mMouseDeltaX = 0.0f;
    float mMouseDeltaY = 0.0f;
};

} // namespace core
