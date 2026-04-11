#include "App.h"
#include "DuckGameplay.h"
#include "HunterGameplay.h"
#include "SceneLayout.h"

#include <algorithm>
#include <array>
#include <vector>

namespace core
{
namespace
{

/*
 * ثوابت آثار القدمين معزولة هنا لأنها تخص العرض الأرضي فقط
 * ولا نحتاجها في ملفات اللعب الأخرى.
 */
constexpr float kFootprintDecayPerSecond = 1.85f;
constexpr float kFootprintFollowPerSecond = 18.0f;
constexpr float kFootContactThreshold = 0.08f;

} // namespace

/*
 * هذا الملف يجمع كل ما يخص تجهيز طبقات العرض قبل الرسم النهائي:
 * - الأرض والعشب
 * - الصياد والبطة
 * - تفاعل القدمين مع الأرض
 */

void App::updateSoilRenderData()
{
    if (mSoilLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mSoilLayoutWidth = 0;
        mSoilLayoutHeight = 0;
        mVulkan.clearTexturedLayer(mSoilLayerId);
        return;
    }

    if (scene::sameWindowLayout(metrics, mSoilLayoutWidth, mSoilLayoutHeight))
    {
        return;
    }

    mSoilLayoutWidth = metrics.width;
    mSoilLayoutHeight = metrics.height;
    mVulkan.updateTexturedLayer(mSoilLayerId, scene::buildSoilQuad());
}

void App::updateGrassRenderData()
{
    if (mGrassLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mGrassLayoutWidth = 0;
        mGrassLayoutHeight = 0;
        mVulkan.clearTexturedLayer(mGrassLayerId);
        return;
    }
    if (scene::sameWindowLayout(metrics, mGrassLayoutWidth, mGrassLayoutHeight))
    {
        return;
    }

    mGrassLayoutWidth = metrics.width;
    mGrassLayoutHeight = metrics.height;
    const scene::GrassLayout layout = scene::buildGrassLayout(metrics);

    std::vector<gfx::TexturedQuad> quads;
    quads.reserve(scene::kMaxGrassQuads);

    constexpr std::array<scene::GrassDepthLayer, 3> GRASS_LAYERS = {
        scene::GrassDepthLayer::Background,
        scene::GrassDepthLayer::Midground,
        scene::GrassDepthLayer::Foreground,
    };

    for (scene::GrassDepthLayer layer : GRASS_LAYERS)
    {
        for (int i = 0; i < layout.tileCount && quads.size() < scene::kMaxGrassQuads; ++i)
        {
            scene::appendGrassQuads(quads, layout, i, layer);
        }
    }

    mVulkan.updateTexturedLayer(mGrassLayerId, quads);
}

void App::updateNatureRenderData()
{
    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mEnvironmentRenderData.cloudInstances.clear();
        mEnvironmentRenderData.backgroundTreeInstances.clear();
        mEnvironmentRenderData.foregroundTreeInstances.clear();
        mVulkan.updateEnvironmentBatches(mEnvironmentRenderData.cloudInstances,
                                         mEnvironmentRenderData.backgroundTreeInstances,
                                         mEnvironmentRenderData.foregroundTreeInstances);
        return;
    }

    mNatureSystem.buildRenderData(metrics, mEnvironmentRenderData);
    mVulkan.updateEnvironmentBatches(mEnvironmentRenderData.cloudInstances,
                                     mEnvironmentRenderData.backgroundTreeInstances,
                                     mEnvironmentRenderData.foregroundTreeInstances);
}

void App::updateHunterRenderData()
{
    if (mHunterLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    if (mHunterState.currentClip.empty())
    {
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        return;
    }

    const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
    if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
    {
        mVulkan.clearTexturedLayer(mHunterLayerId);
        return;
    }

    gfx::UvRect uv;
    mSpriteAnim.getFrameUV(mHunterState.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
    if (mHunterState.flipX)
    {
        std::swap(uv.u0, uv.u1);
    }

    const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
    const float clampMin = -1.0f + logicalHalfWidth;
    const float clampMax = 1.0f - logicalHalfWidth;
    if (clampMin < clampMax)
    {
        mHunterX = std::clamp(mHunterX, clampMin, clampMax);
    }

    gfx::TexturedQuad quad = scene::buildHunterQuad(*frame, mHunterX, metrics);
    quad.uv = uv;
    mVulkan.updateTexturedLayer(mHunterLayerId, quad);
}

void App::updateDuckRenderData()
{
    if (mDuckLayerId == gfx::VulkanContext::INVALID_LAYER_ID)
    {
        return;
    }

    if (mDuckState.currentClip.empty())
    {
        mVulkan.clearTexturedLayer(mDuckLayerId);
        return;
    }

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    if (!metrics.valid())
    {
        mVulkan.clearTexturedLayer(mDuckLayerId);
        return;
    }

    const game::AtlasFrame* frame = mDuckAnim.getFrame(mDuckState.currentFrameIndex);
    if (frame == nullptr || frame->sourceW <= 0 || frame->sourceH <= 0)
    {
        mVulkan.clearTexturedLayer(mDuckLayerId);
        return;
    }

    gfx::UvRect uv;
    mDuckAnim.getFrameUV(mDuckState.currentFrameIndex, uv.u0, uv.u1, uv.v0, uv.v1);
    if (mDuckState.flipX)
    {
        std::swap(uv.u0, uv.u1);
    }
    if (mDuckFlipY)
    {
        std::swap(uv.v0, uv.v1);
    }

    gfx::TexturedQuad quad = scene::buildSpriteQuad(*frame, mDuckX, mDuckY, duckplay::kDuckScreenHalfHeight, metrics);
    quad.uv = uv;
    quad.alpha = mDuckAlpha;
    quad.rotationRadians = mDuckRotation;
    mVulkan.updateTexturedLayer(mDuckLayerId, quad);
}

void App::updateGroundInteraction(float deltaTime)
{
    mLeftGroundPressure = std::max(0.0f, mLeftGroundPressure - deltaTime * kFootprintDecayPerSecond);
    mRightGroundPressure = std::max(0.0f, mRightGroundPressure - deltaTime * kFootprintDecayPerSecond);

    const bool walking = hunterplay::isWalkingClip(mHunterState);

    const scene::WindowMetrics metrics = scene::makeWindowMetrics(mWindow.getWidth(), mWindow.getHeight());
    const game::AtlasFrame* frame = mSpriteAnim.getFrame(mHunterState.currentFrameIndex);
    if (walking && metrics.valid() && frame != nullptr && frame->sourceW > 0 && frame->sourceH > 0)
    {
        const float logicalHalfWidth = scene::hunterLogicalHalfWidth(*frame, metrics);
        const float hunterWidth = logicalHalfWidth * 2.0f;
        const float gait = scene::resolveGait(mHunterState.elapsed);
        const float leftTargetPressure = scene::resolvePressure(gait, true);
        const float rightTargetPressure = scene::resolvePressure(gait, false);
        const float leftFootTargetX = scene::resolveFootTargetX(mHunterX, hunterWidth, gait, true);
        const float rightFootTargetX = scene::resolveFootTargetX(mHunterX, hunterWidth, gait, false);
        const float followFactor = std::clamp(deltaTime * kFootprintFollowPerSecond, 0.0f, 1.0f);

        mGroundFootRadius = std::max(hunterWidth * 0.16f, 0.05f);

        if (leftTargetPressure > kFootContactThreshold)
        {
            if (mLeftGroundPressure <= kFootContactThreshold)
            {
                mLeftGroundX = leftFootTargetX;
            }
            mLeftGroundX += (leftFootTargetX - mLeftGroundX) * followFactor;
            mLeftGroundPressure = std::max(mLeftGroundPressure, leftTargetPressure * leftTargetPressure);
        }

        if (rightTargetPressure > kFootContactThreshold)
        {
            if (mRightGroundPressure <= kFootContactThreshold)
            {
                mRightGroundX = rightFootTargetX;
            }
            mRightGroundX += (rightFootTargetX - mRightGroundX) * followFactor;
            mRightGroundPressure = std::max(mRightGroundPressure, rightTargetPressure * rightTargetPressure);
        }
    }

    mVulkan.setGroundInteraction(mLeftGroundX, mRightGroundX, mGroundFootRadius, mLeftGroundPressure,
                                 mRightGroundPressure);
}

void App::render()
{
    mVulkan.drawFrame();
}

} // namespace core
