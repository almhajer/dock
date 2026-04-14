#include "TextRendererBridge.h"

#include <algorithm>
#include <cmath>

namespace textsys
{
namespace
{

[[nodiscard]] std::uint32_t packColor(const TextColor& color)
{
    const auto toByte = [](float value) -> std::uint32_t {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return static_cast<std::uint32_t>(std::round(clamped * 255.0f));
    };

    return (toByte(color.a) << 24u) | (toByte(color.b) << 16u) | (toByte(color.g) << 8u) | toByte(color.r);
}

} // namespace

std::vector<GlyphQuad> TextRendererBridge::buildGlyphQuads(const TextLayout& layout, const TextRenderStyle& style)
{
    std::vector<GlyphQuad> quads;
    quads.reserve(layout.glyphs.size());

    const std::uint32_t packedColor = packColor(style.color);

    for (const PositionedGlyph& positionedGlyph : layout.glyphs)
    {
        const GlyphMetrics& metrics = positionedGlyph.metrics;
        if (!metrics.hasRenderableQuad || !positionedGlyph.glyph.visible)
        {
            continue;
        }

        const float x0 = style.originX + positionedGlyph.penX + metrics.bearingX * style.scale;
        const float y0 = style.originY + positionedGlyph.penY - metrics.bearingY * style.scale;
        const float x1 = x0 + metrics.width * style.scale;
        const float y1 = y0 + metrics.height * style.scale;

        GlyphQuad quad;
        quad.glyphCodepoint = positionedGlyph.glyph.glyphCodepoint;
        quad.x0 = x0;
        quad.y0 = y0;
        quad.x1 = x1;
        quad.y1 = y1;
        quad.u0 = metrics.u0;
        quad.v0 = metrics.v0;
        quad.u1 = metrics.u1;
        quad.v1 = metrics.v1;

        const std::uint32_t glyphId = static_cast<std::uint32_t>(positionedGlyph.glyph.glyphCodepoint);
        quad.vertices = {{
            {x0, y0, metrics.u0, metrics.v0, packedColor, glyphId},
            {x1, y0, metrics.u1, metrics.v0, packedColor, glyphId},
            {x1, y1, metrics.u1, metrics.v1, packedColor, glyphId},
            {x0, y0, metrics.u0, metrics.v0, packedColor, glyphId},
            {x1, y1, metrics.u1, metrics.v1, packedColor, glyphId},
            {x0, y1, metrics.u0, metrics.v1, packedColor, glyphId},
        }};

        quads.push_back(quad);
    }

    return quads;
}

std::vector<TextVertex> TextRendererBridge::flattenVertices(const std::vector<GlyphQuad>& quads)
{
    std::vector<TextVertex> vertices;
    vertices.reserve(quads.size() * 6u);

    for (const GlyphQuad& quad : quads)
    {
        vertices.insert(vertices.end(), quad.vertices.begin(), quad.vertices.end());
    }

    return vertices;
}

} // namespace textsys
