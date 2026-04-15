/**
 * @file SceneLayout.cpp
 * @brief تنفيذ تخطيط المشهد.
 * @details حساب مواقع العناصر في المشهد.
 */

#include "SceneLayout.h"

#include <algorithm>
#include <cmath>

namespace core::scene
{
namespace
{

constexpr float HUNTER_SCREEN_HALF_HEIGHT = 0.5f;
constexpr float HUNTER_BOTTOM_MARGIN = 0.04f;
constexpr float GRASS_SCREEN_HEIGHT = 0.31f;
constexpr float SOIL_SCREEN_HEIGHT = 0.115f;
constexpr float GRASS_OVERLAP_RATIO = 0.52f;
constexpr float GRASS_CLUMP_ASPECT = 1.02f;

float hash01(float value)
{
    const float raw = std::sin(value * 127.1f + 311.7f) * 43758.5453f;
    return raw - std::floor(raw);
}

float smooth01(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

float noise1D(float value)
{
    const float base = std::floor(value);
    const float fraction = value - base;
    return std::lerp(hash01(base + 0.19f), hash01(base + 1.19f), smooth01(fraction));
}

gfx::TexturedQuad makeBottomAlignedQuad(float left, float bottom, float width, float height)
{
    return gfx::makeTexturedQuad(gfx::makeScreenRect(left, left + width, bottom - height, bottom));
}

gfx::TexturedQuad makeGrassQuad(float left, float width, float height, float alpha, float windPhase, float windResponse)
{
    gfx::TexturedQuad quad = makeBottomAlignedQuad(left, 1.0f, width, height);
    quad.alpha = alpha;
    quad.windWeight = 1.0f;
    quad.windPhase = windPhase;
    quad.windResponse = windResponse;
    quad.materialType = gfx::QUAD_MATERIAL_PROCEDURAL_GRASS;
    return quad;
}

struct GrassVariation
{
    float densityField = 1.0f;
    float xDrift = 0.0f;
    float sweep = 0.0f;
    float heightPulse = 1.0f;
    float widthPulse = 1.0f;
    float phaseBack = 0.0f;
    float phaseMid = 0.0f;
    float phaseFront = 0.0f;
    float responseBack = 0.0f;
    float responseMid = 0.0f;
    float responseFront = 0.0f;
};

GrassVariation buildGrassVariation(float clumpIndex, float tileStep)
{
    const float randomA = hash01(clumpIndex + 1.37f);
    const float randomB = hash01(clumpIndex * 1.91f + 4.12f);
    const float randomC = hash01(clumpIndex * 2.73f + 9.81f);
    const float randomD = hash01(clumpIndex * 4.07f + 2.11f);
    const float densityField = 0.46f + noise1D(clumpIndex * 0.29f + 2.8f) * 0.86f;
    const float moistureField = noise1D(clumpIndex * 0.57f + 8.4f);
    const float spacingField = noise1D(clumpIndex * 0.83f + 14.6f);

    GrassVariation variation;
    variation.densityField = densityField;
    variation.xDrift = ((randomA - 0.5f) * 0.78f + (spacingField - 0.5f) * 0.64f) * tileStep;
    variation.sweep = ((randomB - 0.5f) * 0.86f + (moistureField - 0.5f) * 0.42f) * tileStep;
    variation.heightPulse = 0.82f + randomC * 0.44f;
    variation.widthPulse = 0.74f + randomD * 0.54f;
    variation.phaseBack = clumpIndex * 0.41f + randomA * 5.6f + moistureField * 1.8f;
    variation.phaseMid = clumpIndex * 0.58f + randomB * 7.5f + spacingField * 2.4f;
    variation.phaseFront = clumpIndex * 0.73f + randomC * 9.2f + densityField * 2.8f;
    variation.responseBack = 0.54f + densityField * 0.10f;
    variation.responseMid = 0.72f + densityField * 0.17f + randomD * 0.04f;
    variation.responseFront = 0.92f + densityField * 0.22f + moistureField * 0.08f;
    return variation;
}

void appendGrassPatch(std::vector<gfx::TexturedQuad>& quads, float centerX, float width, float height, float alpha,
                      float windPhase, float windResponse, std::size_t maxQuads)
{
    if (quads.size() >= maxQuads)
    {
        return;
    }

    quads.push_back(makeGrassQuad(centerX - width * 0.5f, width, height, alpha, windPhase, windResponse));
}

} // namespace

WindowMetrics makeWindowMetrics(uint32_t width, uint32_t height)
{
    WindowMetrics metrics;
    metrics.width = width;
    metrics.height = height;
    metrics.widthF = static_cast<float>(width);
    metrics.heightF = static_cast<float>(height);
    if (height > 0)
    {
        metrics.aspect = metrics.widthF / metrics.heightF;
    }
    return metrics;
}

bool sameWindowLayout(const WindowMetrics& metrics, uint32_t cachedWidth, uint32_t cachedHeight)
{
    return metrics.width == cachedWidth && metrics.height == cachedHeight;
}

float hunterLogicalHalfWidth(const game::AtlasFrame& frame, const WindowMetrics& metrics)
{
    return spriteLogicalHalfWidth(frame, metrics, HUNTER_SCREEN_HALF_HEIGHT);
}

float spriteLogicalHalfWidth(const game::AtlasFrame& frame, const WindowMetrics& metrics, float logicalHalfHeight)
{
    return logicalHalfHeight * (static_cast<float>(frame.sourceW) / static_cast<float>(frame.sourceH)) / metrics.aspect;
}

float groundSurfaceY()
{
    // السطح المرئي للأرض يقع فوق شريط التربة بقليل حتى تختفي قواعد العناصر داخله.
    return 1.0f - SOIL_SCREEN_HEIGHT * 0.18f;
}

float grassTopY()
{
    return 1.0f - GRASS_SCREEN_HEIGHT;
}

gfx::TexturedQuad buildSoilQuad()
{
    return gfx::makeTexturedQuad(gfx::makeScreenRect(-1.02f, 1.02f, 1.0f - SOIL_SCREEN_HEIGHT, 1.0f), gfx::fullUvRect(),
                                 1.0f, 0.0f, 0.0f, 0.0f, gfx::QUAD_MATERIAL_PROCEDURAL_SOIL);
}

gfx::TexturedQuad buildSpriteQuad(const game::AtlasFrame& frame, float pivotScreenX, float pivotScreenY,
                                  float logicalHalfHeight, const WindowMetrics& metrics)
{
    const float logicalHeight = logicalHalfHeight * 2.0f;
    const float logicalHalfWidth = spriteLogicalHalfWidth(frame, metrics, logicalHalfHeight);
    const float logicalWidth = logicalHalfWidth * 2.0f;

    const float srcLeft = pivotScreenX - frame.pivotX * logicalWidth;
    const float srcTopY = pivotScreenY - frame.pivotY * logicalHeight;

    /*
     * visualOffset يسمح بتصحيح baseline أو تمركز pose على مستوى بيانات الأطلس،
     * من دون تلويث منطق الحركة أو كود Vulkan بأي ترقيعات حالة.
     */
    const float spriteNormX =
        (static_cast<float>(frame.spriteX) + frame.visualOffsetXPx) / static_cast<float>(frame.sourceW);
    const float spriteNormY =
        (static_cast<float>(frame.spriteY) + frame.visualOffsetYPx) / static_cast<float>(frame.sourceH);
    const float spriteNormW = static_cast<float>(frame.width) / static_cast<float>(frame.sourceW);
    const float spriteNormH = static_cast<float>(frame.height) / static_cast<float>(frame.sourceH);

    const float x0 = srcLeft + spriteNormX * logicalWidth;
    const float x1 = x0 + spriteNormW * logicalWidth;
    const float y0 = srcTopY + spriteNormY * logicalHeight;
    const float y1 = y0 + spriteNormH * logicalHeight;

    return gfx::makeTexturedQuad(gfx::makeScreenRect(x0, x1, y0, y1));
}

gfx::TexturedQuad buildHunterQuad(const game::AtlasFrame& frame, float hunterScreenX, const WindowMetrics& metrics)
{
    const float groundY = 1.0f - HUNTER_BOTTOM_MARGIN * 2.0f;
    return buildSpriteQuad(frame, hunterScreenX, groundY, HUNTER_SCREEN_HALF_HEIGHT, metrics);
}

GrassLayout buildGrassLayout(const WindowMetrics& metrics)
{
    GrassLayout layout;
    layout.tileHeight = GRASS_SCREEN_HEIGHT;
    layout.tileWidth = layout.tileHeight * GRASS_CLUMP_ASPECT / metrics.aspect;
    layout.tileStep = std::max(layout.tileWidth * GRASS_OVERLAP_RATIO, 0.0001f);
    const float overflowPadding = layout.tileWidth;
    layout.startX = -1.0f - overflowPadding;
    const float coverageWidth = 2.0f + overflowPadding * 2.0f;
    layout.tileCount = std::max(1, static_cast<int>(std::ceil(coverageWidth / layout.tileStep)) + 1);
    return layout;
}

void appendGrassQuads(std::vector<gfx::TexturedQuad>& quads, const GrassLayout& layout, int tileIndex,
                      GrassDepthLayer layer, std::size_t maxQuads)
{
    const float clumpIndex = static_cast<float>(tileIndex);
    const GrassVariation variation = buildGrassVariation(clumpIndex, layout.tileStep);
    const float baseCenter = layout.startX + layout.tileStep * clumpIndex + layout.tileWidth * 0.5f;
    const float lushness = variation.densityField;
    const float accentGate = hash01(clumpIndex * 6.13f + 12.7f);
    int stride = 1;

    switch (layer)
    {
    case GrassDepthLayer::Background:
        stride = 3;
        break;
    case GrassDepthLayer::Midground:
        stride = 2;
        break;
    case GrassDepthLayer::Foreground:
        stride = 1;
        break;
    }

    if (tileIndex % stride != 0)
    {
        return;
    }

    switch (layer)
    {
    case GrassDepthLayer::Background:
        appendGrassPatch(quads, baseCenter - layout.tileStep * 0.18f + variation.xDrift * 0.58f,
                         layout.tileWidth * (1.70f + lushness * 0.24f) * variation.widthPulse,
                         layout.tileHeight * (0.54f + lushness * 0.12f) * (0.92f + variation.heightPulse * 0.10f),
                         0.20f + lushness * 0.10f, variation.phaseBack, variation.responseBack, maxQuads);
        break;

    case GrassDepthLayer::Midground:
        appendGrassPatch(quads, baseCenter + variation.xDrift * 0.46f,
                         layout.tileWidth * (1.08f + lushness * 0.20f) * (0.84f + variation.widthPulse * 0.18f),
                         layout.tileHeight * (0.76f + lushness * 0.18f) * (0.92f + variation.heightPulse * 0.16f),
                         0.38f + lushness * 0.14f, variation.phaseMid, variation.responseMid, maxQuads);

        if ((tileIndex % 4 == 0) && (lushness > 0.50f || accentGate > 0.72f))
        {
            appendGrassPatch(quads, baseCenter - layout.tileStep * 0.22f + variation.sweep * 0.24f,
                             layout.tileWidth * (0.82f + lushness * 0.14f) * variation.widthPulse,
                             layout.tileHeight * (0.72f + lushness * 0.18f) * (0.94f + variation.heightPulse * 0.12f),
                             0.34f + lushness * 0.10f, variation.phaseMid + 2.7f, variation.responseMid * 1.02f,
                             maxQuads);
        }
        break;

    case GrassDepthLayer::Foreground:
        appendGrassPatch(quads, baseCenter + variation.sweep * 0.26f,
                         layout.tileWidth * (0.88f + lushness * 0.16f) * (0.82f + variation.widthPulse * 0.14f),
                         layout.tileHeight * (0.96f + lushness * 0.22f) * (0.98f + variation.heightPulse * 0.16f),
                         0.62f + lushness * 0.12f, variation.phaseFront, variation.responseFront, maxQuads);

        if ((tileIndex % 3 == 0) && (lushness > 0.62f || accentGate > 0.78f))
        {
            appendGrassPatch(quads, baseCenter - layout.tileStep * 0.14f + variation.xDrift * 0.40f,
                             layout.tileWidth * (0.66f + lushness * 0.12f) * (0.80f + variation.widthPulse * 0.10f),
                             layout.tileHeight * (1.02f + lushness * 0.24f) * (1.00f + variation.heightPulse * 0.14f),
                             0.70f + lushness * 0.08f, variation.phaseFront + 3.4f, variation.responseFront * 1.06f,
                             maxQuads);
        }
        break;
    }
}

float resolveGait(float animationTime)
{
    return std::sin(animationTime * 9.5f);
}

float resolvePressure(float gait, bool leftFoot)
{
    return std::max(0.0f, leftFoot ? -gait : gait);
}

float resolveFootTargetX(float hunterScreenX, float hunterLogicalWidth, float gait, bool leftFoot)
{
    const float sideOffset = hunterLogicalWidth * 0.14f * (leftFoot ? -1.0f : 1.0f);
    return hunterScreenX + sideOffset - gait * hunterLogicalWidth * 0.07f;
}

} // namespace core::scene
