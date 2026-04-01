#pragma once

#include <array>
#include <cstddef>

namespace gfx {

inline constexpr float QUAD_MATERIAL_TEXTURED = 0.0f;
inline constexpr float QUAD_MATERIAL_PROCEDURAL_GRASS = 1.0f;
inline constexpr float QUAD_MATERIAL_PROCEDURAL_SOIL = 2.0f;
inline constexpr std::size_t QUAD_VERTICAL_SEGMENTS = 6;
inline constexpr std::size_t QUAD_VERTEX_COUNT = QUAD_VERTICAL_SEGMENTS * 6;

struct UvRect {
    float u0 = 0.0f;
    float u1 = 0.0f;
    float v0 = 0.0f;
    float v1 = 0.0f;
};

struct ScreenRect {
    float x0 = 0.0f;
    float x1 = 0.0f;
    float y0 = 0.0f;
    float y1 = 0.0f;
};

struct TexturedQuad {
    ScreenRect screen;
    UvRect uv;
    float alpha = 1.0f;
    float windWeight = 0.0f;
    float windPhase = 0.0f;
    float windResponse = 0.0f;
    float materialType = QUAD_MATERIAL_TEXTURED;
};

struct QuadVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float alpha = 1.0f;
    float windWeight = 0.0f;
    float windPhase = 0.0f;
    float windResponse = 0.0f;
    float materialType = QUAD_MATERIAL_TEXTURED;
};

[[nodiscard]] inline constexpr UvRect fullUvRect() {
    return {0.0f, 1.0f, 0.0f, 1.0f};
}

[[nodiscard]] inline constexpr ScreenRect makeScreenRect(float x0, float x1, float y0, float y1) {
    return {x0, x1, y0, y1};
}

[[nodiscard]] inline constexpr TexturedQuad makeTexturedQuad(
    ScreenRect screen,
    UvRect uv = fullUvRect(),
    float alpha = 1.0f,
    float windWeight = 0.0f,
    float windPhase = 0.0f,
    float windResponse = 0.0f,
    float materialType = QUAD_MATERIAL_TEXTURED
) {
    return {screen, uv, alpha, windWeight, windPhase, windResponse, materialType};
}

[[nodiscard]] inline float lerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

[[nodiscard]] inline float easedWindWeight(float t) {
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] inline std::array<QuadVertex, QUAD_VERTEX_COUNT> buildQuadVertices(const TexturedQuad& quad) {
    std::array<QuadVertex, QUAD_VERTEX_COUNT> vertices{};
    std::size_t vertexIndex = 0;

    for (std::size_t segment = 0; segment < QUAD_VERTICAL_SEGMENTS; ++segment) {
        const float topT = static_cast<float>(segment) / static_cast<float>(QUAD_VERTICAL_SEGMENTS);
        const float bottomT = static_cast<float>(segment + 1) / static_cast<float>(QUAD_VERTICAL_SEGMENTS);

        const float yTop = lerpFloat(quad.screen.y0, quad.screen.y1, topT);
        const float yBottom = lerpFloat(quad.screen.y0, quad.screen.y1, bottomT);
        const float vTop = lerpFloat(quad.uv.v0, quad.uv.v1, topT);
        const float vBottom = lerpFloat(quad.uv.v0, quad.uv.v1, bottomT);

        const float topWeight = quad.windWeight * easedWindWeight(1.0f - topT);
        const float bottomWeight = quad.windWeight * easedWindWeight(1.0f - bottomT);

        vertices[vertexIndex++] = {quad.screen.x0, yTop, quad.uv.u0, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {quad.screen.x1, yTop, quad.uv.u1, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {quad.screen.x1, yBottom, quad.uv.u1, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {quad.screen.x0, yTop, quad.uv.u0, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {quad.screen.x1, yBottom, quad.uv.u1, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {quad.screen.x0, yBottom, quad.uv.u0, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
    }

    return vertices;
}

} // namespace gfx
