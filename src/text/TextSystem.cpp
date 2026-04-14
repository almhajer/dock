#include "TextSystem.h"

namespace textsys
{

TextSystem::TextSystem(float glyphEmSize)
    : mBidiResolver(), mTextShaper(), mGlyphProvider(glyphEmSize), mLayouter(mBidiResolver, mTextShaper, mGlyphProvider)
{
}

void TextSystem::setGlyphEmSize(float emSize)
{
    mGlyphProvider.setEmSize(emSize);
}

float TextSystem::glyphEmSize() const noexcept
{
    return mGlyphProvider.emSize();
}

TextLayout TextSystem::layoutUtf8(std::string_view utf8Text, const TextLayoutOptions& options) const
{
    return mLayouter.layoutUtf8(utf8Text, options);
}

TextLayout TextSystem::layoutUtf8(std::u8string_view utf8Text, const TextLayoutOptions& options) const
{
    return mLayouter.layoutUtf8(utf8Text, options);
}

TextLayout TextSystem::layoutUnicode(const UnicodeText& text, const TextLayoutOptions& options) const
{
    return mLayouter.layoutUnicode(text, options);
}

std::vector<GlyphQuad> TextSystem::buildGlyphQuads(const TextLayout& layout, const TextRenderStyle& style) const
{
    return TextRendererBridge::buildGlyphQuads(layout, style);
}

std::vector<TextVertex> TextSystem::buildVertices(const TextLayout& layout, const TextRenderStyle& style) const
{
    return TextRendererBridge::flattenVertices(buildGlyphQuads(layout, style));
}

PreparedText TextSystem::prepareUtf8(std::string_view utf8Text, const TextSystemBuildOptions& options) const
{
    PreparedText prepared;
    prepared.layout = layoutUtf8(utf8Text, options.layout);
    prepared.quads = buildGlyphQuads(prepared.layout, options.render);
    prepared.vertices = TextRendererBridge::flattenVertices(prepared.quads);
    return prepared;
}

PreparedText TextSystem::prepareUtf8(std::u8string_view utf8Text, const TextSystemBuildOptions& options) const
{
    PreparedText prepared;
    prepared.layout = layoutUtf8(utf8Text, options.layout);
    prepared.quads = buildGlyphQuads(prepared.layout, options.render);
    prepared.vertices = TextRendererBridge::flattenVertices(prepared.quads);
    return prepared;
}

const IBidiResolver& TextSystem::bidiResolver() const noexcept
{
    return mBidiResolver;
}

const ITextShaper& TextSystem::shaper() const noexcept
{
    return mTextShaper;
}

const IGlyphMetricsProvider& TextSystem::glyphMetricsProvider() const noexcept
{
    return mGlyphProvider;
}

} // namespace textsys
