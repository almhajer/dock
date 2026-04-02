#pragma once

#include "../game/SpriteAtlas.h"
#include "../gfx/RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::scene {

inline constexpr std::size_t kMaxGrassQuads = 240;

enum class GrassDepthLayer {
    Background,
    Midground,
    Foreground,
};

struct WindowMetrics {
    uint32_t width = 0;
    uint32_t height = 0;
    float widthF = 0.0f;
    float heightF = 0.0f;
    float aspect = 1.0f;

    [[nodiscard]] bool valid() const {
        return width > 0 && height > 0 && widthF >= 1.0f && heightF >= 1.0f;
    }
};

struct GrassLayout {
    float tileWidth = 0.0f;
    float tileHeight = 0.0f;
    float tileStep = 0.0f;
    float startX = 0.0f;
    int tileCount = 0;
};

WindowMetrics makeWindowMetrics(uint32_t width, uint32_t height);
bool sameWindowLayout(const WindowMetrics& metrics, uint32_t cachedWidth, uint32_t cachedHeight);
float hunterLogicalHalfWidth(const game::AtlasFrame& frame, const WindowMetrics& metrics);

gfx::TexturedQuad buildSoilQuad();
gfx::TexturedQuad buildHunterQuad(const game::AtlasFrame& frame,
                                  float hunterX,
                                  const WindowMetrics& metrics);

GrassLayout buildGrassLayout(const WindowMetrics& metrics);
void appendGrassQuads(std::vector<gfx::TexturedQuad>& quads,
                      const GrassLayout& layout,
                      int tileIndex,
                      GrassDepthLayer layer,
                      std::size_t maxQuads = kMaxGrassQuads);

float resolveGait(float animationTime);
float resolvePressure(float gait, bool leftFoot);
float resolveFootTargetX(float hunterX, float hunterWidth, float gait, bool leftFoot);

} // namespace core::scene
