#include "InternalGlyphProvider.h"

#include <algorithm>

namespace textsys
{
namespace
{

[[nodiscard]] float resolveAdvanceFactor(const GlyphInfo& glyph)
{
    if (glyph.glyphCodepoint == U' ')
    {
        return 0.38f;
    }

    if (glyph.script == Script::Arabic)
    {
        switch (glyph.form)
        {
        case GlyphForm::Medial:
            return 0.84f;
        case GlyphForm::Initial:
        case GlyphForm::Final:
            return 0.78f;
        case GlyphForm::Ligature:
            return 1.05f;
        case GlyphForm::Isolated:
        case GlyphForm::None:
        default:
            return 0.72f;
        }
    }

    if (glyph.script == Script::Digit)
    {
        return 0.62f;
    }

    if (glyph.script == Script::Latin)
    {
        return 0.58f;
    }

    if (glyph.glyphCodepoint == U'/' || glyph.glyphCodepoint == U'+' || glyph.glyphCodepoint == U'-' ||
        glyph.glyphCodepoint == U'%')
    {
        return 0.46f;
    }

    return 0.52f;
}

[[nodiscard]] GlyphMetrics makePlaceholderMetrics(const GlyphInfo& glyph, float emSize)
{
    GlyphMetrics metrics;
    metrics.height = emSize;
    metrics.bearingY = emSize * 0.82f;
    metrics.width = emSize * resolveAdvanceFactor(glyph);
    metrics.advanceX = metrics.width + emSize * 0.06f;
    metrics.advanceY = 0.0f;

    if (glyph.glyphCodepoint == U' ')
    {
        metrics.width = 0.0f;
        metrics.advanceX = emSize * 0.38f;
        metrics.hasRenderableQuad = false;
        return metrics;
    }

    /*
     * UVs placeholder فقط.
     * نوزع glyphs على grid وهمي 16x16 لتسهيل ربط النتائج مع أي atlas لاحق.
     */
    constexpr float kCellSize = 1.0f / 16.0f;
    const std::uint32_t slot = static_cast<std::uint32_t>(glyph.glyphCodepoint) & 0xFFu;
    const std::uint32_t column = slot % 16u;
    const std::uint32_t row = slot / 16u;
    metrics.u0 = static_cast<float>(column) * kCellSize;
    metrics.v0 = static_cast<float>(row) * kCellSize;
    metrics.u1 = metrics.u0 + kCellSize;
    metrics.v1 = metrics.v0 + kCellSize;
    return metrics;
}

} // namespace

InternalGlyphProvider::InternalGlyphProvider(float emSize)
{
    setEmSize(emSize);
}

void InternalGlyphProvider::setEmSize(float emSize)
{
    mEmSize = std::max(emSize, 0.001f);
}

float InternalGlyphProvider::emSize() const noexcept
{
    return mEmSize;
}

GlyphMetrics InternalGlyphProvider::queryGlyphMetrics(const GlyphInfo& glyph) const
{
    return makePlaceholderMetrics(glyph, mEmSize);
}

} // namespace textsys
