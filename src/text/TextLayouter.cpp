#include "TextLayouter.h"

#include "Utf8.h"

#include <algorithm>

namespace textsys
{
namespace
{

[[nodiscard]] float resolveAlignmentOffset(float width, TextAlignment alignment)
{
    switch (alignment)
    {
    case TextAlignment::Center:
        return -width * 0.5f;
    case TextAlignment::End:
        return -width;
    case TextAlignment::Start:
    default:
        return 0.0f;
    }
}

} // namespace

TextLayouter::TextLayouter(const IBidiResolver& bidiResolver, const ITextShaper& textShaper,
                           const IGlyphMetricsProvider& glyphMetricsProvider)
    : mBidiResolver(bidiResolver), mTextShaper(textShaper), mGlyphMetricsProvider(glyphMetricsProvider)
{
}

TextLayout TextLayouter::layoutUtf8(std::string_view utf8Text, const TextLayoutOptions& options) const
{
    UnicodeText text;
    text.codepoints = utf8::decode(utf8Text);
    return layoutUnicode(text, options);
}

TextLayout TextLayouter::layoutUtf8(std::u8string_view utf8Text, const TextLayoutOptions& options) const
{
    UnicodeText text;
    text.codepoints = utf8::decode(utf8Text);
    return layoutUnicode(text, options);
}

TextLayout TextLayouter::layoutUnicode(const UnicodeText& text, const TextLayoutOptions& options) const
{
    TextLayout layout;
    layout.source = text;
    layout.visualRuns = mBidiResolver.resolveVisualRuns(text);
    layout.baseline = options.baselineY;
    layout.height = options.lineHeight * options.fontScale;

    float penX = options.originX;
    const float penY = options.baselineY;

    for (const TextRun& run : layout.visualRuns)
    {
        std::vector<GlyphInfo> glyphs = mTextShaper.shapeRun(run);
        if (run.direction == TextDirection::RightToLeft)
        {
            std::reverse(glyphs.begin(), glyphs.end());
        }

        for (const GlyphInfo& glyph : glyphs)
        {
            const GlyphMetrics metrics = mGlyphMetricsProvider.queryGlyphMetrics(glyph);

            PositionedGlyph positionedGlyph;
            positionedGlyph.glyph = glyph;
            positionedGlyph.metrics = metrics;
            positionedGlyph.penX = penX;
            positionedGlyph.penY = penY;
            positionedGlyph.advanceX = positionedGlyph.metrics.advanceX * options.fontScale + options.letterSpacing;
            positionedGlyph.advanceY = positionedGlyph.metrics.advanceY * options.fontScale;
            layout.glyphs.push_back(positionedGlyph);

            penX += positionedGlyph.advanceX;
        }
    }

    layout.width = penX - options.originX;

    const float alignmentOffset = resolveAlignmentOffset(layout.width, options.alignment);
    if (alignmentOffset != 0.0f)
    {
        for (PositionedGlyph& glyph : layout.glyphs)
        {
            glyph.penX += alignmentOffset;
        }
    }

    return layout;
}

} // namespace textsys
