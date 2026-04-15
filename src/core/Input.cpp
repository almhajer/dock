/**
 * @file Input.cpp
 * @brief تنفيذ إدارة الإدخال.
 * @details التقاط إدخال المستخدم من GLFW.
 */

#include "Input.h"

#include <GLFW/glfw3.h>
#include <unordered_map>

namespace core
{

namespace
{

std::unordered_map<GLFWwindow*, Input*> gInputInstances;

Input* getInputForWindow(GLFWwindow* window)
{
    auto it = gInputInstances.find(window);
    return (it != gInputInstances.end()) ? it->second : nullptr;
}

} // namespace

/**
 * @brief تهيئة الإدخال.
 * @param window النافذة المرتبطة.
 */
void Input::init(GLFWwindow* window)
{
    mWindow = window;
    gInputInstances[window] = this;

    // ربط ردود نداء GLFW مع هذه النافذة دون لمس user pointer الخاص بالنافذة
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // الحصول على موقع المؤشر الأولي
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mMouseX = mMouseLastX = static_cast<float>(xpos);
    mMouseY = mMouseLastY = static_cast<float>(ypos);
}

/**
 * @brief تحديث حالة الإدخال.
 */
void Input::update()
{
    // نقل الحالة الحالية إلى السابقة
    mKeysPrevious = mKeysCurrent;
    mMouseButtonsPrevious = mMouseButtonsCurrent;

    // حساب إزاحة المؤشر
    mMouseDeltaX = mMouseX - mMouseLastX;
    mMouseDeltaY = mMouseY - mMouseLastY;
    mMouseLastX = mMouseX;
    mMouseLastY = mMouseY;
}

/**
 * @brief إيقاف الإدخال.
 */
void Input::shutdown()
{
    // فصل callbacks عن GLFW قبل تدمير أي شيء
    if (mWindow)
    {
        gInputInstances.erase(mWindow);
        glfwSetKeyCallback(mWindow, nullptr);
        glfwSetMouseButtonCallback(mWindow, nullptr);
        glfwSetCursorPosCallback(mWindow, nullptr);
        mWindow = nullptr;
    }
    mKeysCurrent.clear();
    mKeysPrevious.clear();
    mMouseButtonsCurrent.clear();
    mMouseButtonsPrevious.clear();
}

/**
 * @brief التحقق من مفتاح مضغوط.
 * @param key رمز المفتاح.
 * @return صحيح إذا كان المفتاح مضغوطاً.
 */
bool Input::isKeyPressed(int key) const
{
    auto it = mKeysCurrent.find(key);
    return it != mKeysCurrent.end() && it->second;
}

/**
 * @brief التحقق من مفتاح للتو تم ضغطه.
 * @param key رمز المفتاح.
 * @return صحيح إذا تم الضغط على المفتاح للتو.
 */
bool Input::isKeyJustPressed(int key) const
{
    bool current = mKeysCurrent.count(key) ? mKeysCurrent.at(key) : false;
    bool previous = mKeysPrevious.count(key) ? mKeysPrevious.at(key) : false;
    return current && !previous;
}

/**
 * @brief التحقق من زر الماوس مضغوط.
 * @param button رمز الزر.
 * @return صحيح إذا كان الزر مضغوطاً.
 */
bool Input::isMouseButtonPressed(int button) const
{
    auto it = mMouseButtonsCurrent.find(button);
    return it != mMouseButtonsCurrent.end() && it->second;
}

/**
 * @brief التحقق من زر للتو تم الضغط عليه.
 * @param button رمز الزر.
 * @return صحيح إذا تم الضغط على الزر للتو.
 */
bool Input::isMouseButtonJustPressed(int button) const
{
    bool current = mMouseButtonsCurrent.count(button) ? mMouseButtonsCurrent.at(button) : false;
    bool previous = mMouseButtonsPrevious.count(button) ? mMouseButtonsPrevious.at(button) : false;
    return current && !previous;
}

/**
 * @brief الحصول على إحداثي X للماوس.
 * @return إحداثي X.
 */
float Input::getMouseX() const
{
    return mMouseX;
}

/**
 * @brief الحصول على إحداثي Y للماوس.
 * @return إحداثي Y.
 */
float Input::getMouseY() const
{
    return mMouseY;
}

/**
 * @brief الحصول على تغيير X للماوس.
 * @return تغيير X.
 */
float Input::getMouseDeltaX() const
{
    return mMouseDeltaX;
}

/**
 * @brief الحصول على تغيير Y للماوس.
 * @return تغيير Y.
 */
float Input::getMouseDeltaY() const
{
    return mMouseDeltaY;
}

// ─── ردود نداء GLFW (ثابتة) ────────────────────────────────────

void Input::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto* input = getInputForWindow(window);
    if (!input || key < 0)
        return;

    if (action == GLFW_PRESS)
    {
        input->mKeysCurrent[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        input->mKeysCurrent[key] = false;
    }
    // GLFW_REPEAT لا نغير فيه شيئًا - يبقى مضغوطًا
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    auto* input = getInputForWindow(window);
    if (!input)
        return;

    if (action == GLFW_PRESS)
    {
        input->mMouseButtonsCurrent[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        input->mMouseButtonsCurrent[button] = false;
    }
}

void Input::cursorPosCallback(GLFWwindow* window, double cursorX, double cursorY)
{
    auto* input = getInputForWindow(window);
    if (!input)
        return;

    input->mMouseX = static_cast<float>(cursorX);
    input->mMouseY = static_cast<float>(cursorY);
}

} // namespace core
