#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace textsys
{

struct TextColor
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

/*
 * Vertex مستقل عن Vulkan، لكنه جاهز للنسخ إلى vertex buffer.
 */
struct TextVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    std::uint32_t colorRgba8 = 0xFFFFFFFFu;
    std::uint32_t glyphId = 0u;
};

/*
 * quad واحد لكل glyph مع 6 رؤوس جاهزة لـ triangle list.
 */
struct GlyphQuad
{
    char32_t glyphCodepoint = U'\0';
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    std::array<TextVertex, 6> vertices{};
};

/*
 * إعدادات تحويل الـ layout إلى quads مرسومة.
 */
struct TextRenderStyle
{
    float originX = 0.0f;
    float originY = 0.0f;
    float scale = 1.0f;
    TextColor color = {};
};

} // namespace textsys
