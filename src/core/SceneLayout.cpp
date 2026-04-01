#include "SceneLayout.h"

#include <algorithm>
#include <cmath>

namespace core::scene {
namespace {

constexpr float HUNTER_SCREEN_HALF_HEIGHT = 0.5f;
constexpr float HUNTER_BOTTOM_MARGIN = 0.04f;
constexpr float GRASS_SCREEN_HEIGHT = 0.30f;
constexpr float SOIL_SCREEN_HEIGHT = 0.115f;
constexpr float GRASS_OVERLAP_RATIO = 0.48f;
constexpr float GRASS_CLUMP_ASPECT = 0.92f;

float hash01(float value)
{
    const float raw = std::sin(value * 127.1f + 311.7f) * 43758.5453f;
    return raw - std::floor(raw);
}

gfx::TexturedQuad makeBottomAlignedQuad(float left, float bottom, float width, float height)
{
    return gfx::makeTexturedQuad(
        gfx::makeScreenRect(left, left + width, bottom - height, bottom));
}

gfx::TexturedQuad makeGrassQuad(float left,
                                float width,
                                float height,
                                float alpha,
                                float windPhase,
                                float windResponse)
{
    gfx::TexturedQuad quad = makeBottomAlignedQuad(left, 1.0f, width, height);
    quad.alpha = alpha;
    quad.windWeight = 1.0f;
    quad.windPhase = windPhase;
    quad.windResponse = windResponse;
    quad.materialType = gfx::QUAD_MATERIAL_PROCEDURAL_GRASS;
    return quad;
}

struct GrassVariation {
    float heightScale = 1.0f;
    float widthScale = 1.0f;
    float xJitter = 0.0f;
    float alpha = 1.0f;
    float backAlpha = 1.0f;
    float phaseA = 0.0f;
    float phaseB = 0.0f;
    float responseA = 0.0f;
    float responseB = 0.0f;
    float widthJitterA = 1.0f;
    float widthJitterB = 1.0f;
    float heightJitter = 1.0f;
};

GrassVariation buildGrassVariation(float clumpIndex, float tileStep)
{
    const float randomA = hash01(clumpIndex + 1.37f);
    const float randomB = hash01(clumpIndex * 1.91f + 4.12f);
    const float randomC = hash01(clumpIndex * 2.73f + 9.81f);
    const float randomD = hash01(clumpIndex * 4.07f + 2.11f);

    GrassVariation variation;
    variation.heightScale = 0.96f + randomA * 0.10f;
    variation.widthScale = 0.76f + randomB * 0.34f;
    variation.xJitter = (randomC - 0.5f) * tileStep * 0.08f;
    variation.alpha = 0.86f + randomB * 0.10f;
    variation.backAlpha = 0.52f + randomD * 0.18f;
    variation.phaseA = clumpIndex * 0.47f + randomD * 7.0f;
    variation.phaseB = clumpIndex * 0.61f + randomA * 5.0f + randomC * 1.7f;
    variation.responseA = 0.62f + variation.heightScale * 0.30f;
    variation.responseB = 0.74f + variation.heightScale * 0.38f + randomB * 0.08f;
    variation.widthJitterA = 0.88f + randomD * 0.22f;
    variation.widthJitterB = 0.84f + randomA * 0.28f;
    variation.heightJitter = 0.98f + randomC * 0.04f;
    return variation;
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
    return HUNTER_SCREEN_HALF_HEIGHT *
           (static_cast<float>(frame.sourceWidth) /
            static_cast<float>(frame.sourceHeight)) /
           metrics.aspect;
}

gfx::TexturedQuad buildSoilQuad()
{
    return gfx::makeTexturedQuad(
        gfx::makeScreenRect(-1.02f, 1.02f, 1.0f - SOIL_SCREEN_HEIGHT, 1.0f),
        gfx::fullUvRect(),
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        gfx::QUAD_MATERIAL_PROCEDURAL_SOIL);
}

gfx::TexturedQuad buildHunterQuad(const game::AtlasFrame& frame,
                                  float hunterX,
                                  const WindowMetrics& metrics)
{
    const float logicalHalfHeight = HUNTER_SCREEN_HALF_HEIGHT;
    const float logicalHeight = logicalHalfHeight * 2.0f;
    const float logicalHalfWidth = hunterLogicalHalfWidth(frame, metrics);
    const float logicalX0 = hunterX - logicalHalfWidth;
    const float logicalY0 = 1.0f - HUNTER_BOTTOM_MARGIN * 2.0f - logicalHeight;
    const float logicalWidth = logicalHalfWidth * 2.0f;
    const float x0 = logicalX0 + logicalWidth *
        (static_cast<float>(frame.offsetX) / static_cast<float>(frame.sourceWidth));
    const float x1 = logicalX0 + logicalWidth *
        (static_cast<float>(frame.offsetX + frame.width) / static_cast<float>(frame.sourceWidth));
    const float y0 = logicalY0 + logicalHeight *
        (static_cast<float>(frame.offsetY) / static_cast<float>(frame.sourceHeight));
    const float y1 = logicalY0 + logicalHeight *
        (static_cast<float>(frame.offsetY + frame.height) / static_cast<float>(frame.sourceHeight));

    return gfx::makeTexturedQuad(gfx::makeScreenRect(x0, x1, y0, y1));
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
    layout.tileCount = std::max(
        1,
        static_cast<int>(std::ceil(coverageWidth / layout.tileStep)) + 1);
    return layout;
}

void appendGrassQuads(std::vector<gfx::TexturedQuad>& quads,
                      const GrassLayout& layout,
                      int tileIndex,
                      std::size_t maxQuads)
{
    const float clumpIndex = static_cast<float>(tileIndex);
    const GrassVariation variation = buildGrassVariation(clumpIndex, layout.tileStep);

    quads.push_back(makeGrassQuad(
        layout.startX + layout.tileStep * clumpIndex - layout.tileStep * 0.12f + variation.xJitter * 0.45f,
        layout.tileWidth * (variation.widthScale * variation.widthJitterA),
        layout.tileHeight * (variation.heightScale * variation.heightJitter),
        variation.backAlpha,
        variation.phaseA,
        variation.responseA));

    if (quads.size() >= maxQuads)
    {
        return;
    }

    quads.push_back(makeGrassQuad(
        layout.startX + layout.tileStep * clumpIndex + variation.xJitter,
        layout.tileWidth * (variation.widthScale * variation.widthJitterB),
        layout.tileHeight * variation.heightScale,
        variation.alpha,
        variation.phaseB,
        variation.responseB));
}

float resolveGait(float animationTime)
{
    return std::sin(animationTime * 9.5f);
}

float resolvePressure(float gait, bool leftFoot)
{
    return std::max(0.0f, leftFoot ? -gait : gait);
}

float resolveFootTargetX(float hunterX, float hunterWidth, float gait, bool leftFoot)
{
    const float sideOffset = hunterWidth * 0.14f * (leftFoot ? -1.0f : 1.0f);
    return hunterX + sideOffset - gait * hunterWidth * 0.07f;
}

} // namespace core::scene
