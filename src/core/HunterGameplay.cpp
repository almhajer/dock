#include "HunterGameplay.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace core::hunterplay
{
namespace
{

/*
 @brief اتجاه المشي يتبع المفاتيح مباشرة عند وجود إدخال حركة.
 @param moveIntent نية الحركة من resolveMoveIntent.
 @param fallbackFacingLeft الاتجاه الافتراضي عند السكون.
 @return true إذا كان الصياد يتجه يساراً.
 */
[[nodiscard]] bool resolveFacingLeftFromMoveIntent(int moveIntent, bool fallbackFacingLeft)
{
    if (moveIntent < 0)
    {
        return true;
    }
    if (moveIntent > 0)
    {
        return false;
    }
    return fallbackFacingLeft;
}

} // namespace

int resolveMoveIntent(const Input& input)
{
    int moveIntent = 0;
    if (input.isKeyPressed(GLFW_KEY_A) || input.isKeyPressed(GLFW_KEY_LEFT))
    {
        --moveIntent;
    }
    if (input.isKeyPressed(GLFW_KEY_D) || input.isKeyPressed(GLFW_KEY_RIGHT))
    {
        ++moveIntent;
    }
    return std::clamp(moveIntent, -1, 1);
}

bool resolveFacingLeftFromCursor(const CursorScreenPosition& cursor, float hunterScreenX, bool fallbackFacingLeft)
{
    if (!cursor.valid)
    {
        return fallbackFacingLeft;
    }

    if (cursor.x < hunterScreenX)
    {
        return true;
    }
    if (cursor.x > hunterScreenX)
    {
        return false;
    }
    return fallbackFacingLeft;
}

bool resolveIdleFacingLeft(int moveIntent, const CursorScreenPosition& cursor, float hunterScreenX,
                           bool fallbackFacingLeft)
{
    if (moveIntent != 0)
    {
        return resolveFacingLeftFromMoveIntent(moveIntent, fallbackFacingLeft);
    }

    return resolveFacingLeftFromCursor(cursor, hunterScreenX, fallbackFacingLeft);
}

void updateReloadState(bool& isReloading, float& reloadTimer, float deltaTime)
{
    if (!isReloading)
    {
        return;
    }

    reloadTimer = std::max(0.0f, reloadTimer - deltaTime);
    if (reloadTimer <= 0.0f)
    {
        isReloading = false;
    }
}

bool isWalkingClip(const game::AnimationState& state)
{
    return state.currentClip == "walk_left" || state.currentClip == "walk_right";
}

void holdHunterOnShotStartFrame(game::AnimationState& state)
{
    const game::AnimationClip* clip = state.activeClip;
    if (clip == nullptr || clip->frames.empty())
    {
        return;
    }

    state.currentFrameIndex = clip->frames.front();
    state.finished = true;
}

CursorScreenPosition resolveCursorScreenPosition(const Input& input, GLFWwindow* window)
{
    CursorScreenPosition cursor;
    if (window == nullptr)
    {
        return cursor;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        return cursor;
    }

    const float clampedMouseX = std::clamp(input.getMouseX(), 0.0f, static_cast<float>(windowWidth));
    const float clampedMouseY = std::clamp(input.getMouseY(), 0.0f, static_cast<float>(windowHeight));

    cursor.x = (clampedMouseX / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
    cursor.y = (clampedMouseY / static_cast<float>(windowHeight)) * 2.0f - 1.0f;
    cursor.valid = true;
    return cursor;
}

bool shouldUseHighShootPose(const Input& input, GLFWwindow* window, float hunterScreenX,
                            const scene::WindowMetrics& metrics, const game::AtlasFrame* frame)
{
    if (!metrics.valid() || frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
    {
        return false;
    }

    const CursorScreenPosition cursor = resolveCursorScreenPosition(input, window);
    if (!cursor.valid)
    {
        return false;
    }

    const gfx::TexturedQuad hunterQuad = scene::buildHunterQuad(*frame, hunterScreenX, metrics);
    const float aimOriginX = hunterScreenX;
    const float aimOriginY = hunterQuad.screen.y0 + (hunterQuad.screen.y1 - hunterQuad.screen.y0) * 0.34f;
    const float dx = cursor.x - aimOriginX;
    const float dy = cursor.y - aimOriginY;

    return dy < -0.08f && (-dy) > std::abs(dx) * 1.35f;
}

} // namespace core::hunterplay
