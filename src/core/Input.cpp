#include "Input.h"

#include <GLFW/glfw3.h>

namespace core {

void Input::init(GLFWwindow* window) {
    mWindow = window;

    // ربط ردود نداء GLFW مع هذه النافذة
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // الحصول على موقع المؤشر الأولي
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mMouseX = mMouseLastX = static_cast<float>(xpos);
    mMouseY = mMouseLastY = static_cast<float>(ypos);
}

void Input::update() {
    // نقل الحالة الحالية إلى السابقة
    mKeysPrevious = mKeysCurrent;
    mMouseButtonsPrevious = mMouseButtonsCurrent;

    // حساب إزاحة المؤشر
    mMouseDeltaX = mMouseX - mMouseLastX;
    mMouseDeltaY = mMouseY - mMouseLastY;
    mMouseLastX = mMouseX;
    mMouseLastY = mMouseY;
}

bool Input::isKeyPressed(int key) const {
    auto it = mKeysCurrent.find(key);
    return it != mKeysCurrent.end() && it->second;
}

bool Input::isKeyJustPressed(int key) const {
    bool current = mKeysCurrent.count(key) ? mKeysCurrent.at(key) : false;
    bool previous = mKeysPrevious.count(key) ? mKeysPrevious.at(key) : false;
    return current && !previous;
}

bool Input::isMouseButtonPressed(int button) const {
    auto it = mMouseButtonsCurrent.find(button);
    return it != mMouseButtonsCurrent.end() && it->second;
}

bool Input::isMouseButtonJustPressed(int button) const {
    bool current = mMouseButtonsCurrent.count(button) ? mMouseButtonsCurrent.at(button) : false;
    bool previous = mMouseButtonsPrevious.count(button) ? mMouseButtonsPrevious.at(button) : false;
    return current && !previous;
}

float Input::getMouseX() const { return mMouseX; }
float Input::getMouseY() const { return mMouseY; }
float Input::getMouseDeltaX() const { return mMouseDeltaX; }
float Input::getMouseDeltaY() const { return mMouseDeltaY; }

// ─── ردود نداء GLFW (ثابتة) ────────────────────────────────────

void Input::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input || key < 0) return;

    if (action == GLFW_PRESS) {
        input->mKeysCurrent[key] = true;
    } else if (action == GLFW_RELEASE) {
        input->mKeysCurrent[key] = false;
    }
    // GLFW_REPEAT لا نغير فيه شيئًا - يبقى مضغوطًا
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    if (action == GLFW_PRESS) {
        input->mMouseButtonsCurrent[button] = true;
    } else if (action == GLFW_RELEASE) {
        input->mMouseButtonsCurrent[button] = false;
    }
}

void Input::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    input->mMouseX = static_cast<float>(xpos);
    input->mMouseY = static_cast<float>(ypos);
}

} // namespace core
